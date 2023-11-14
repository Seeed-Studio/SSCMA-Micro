#pragma once

#include <cstring>
#include <string>

#include "mqtt.hpp"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::types;
using namespace sscma::utility;

void set_wireless_network(const std::vector<std::string>& argv, bool has_reply = true) {
    // crate config from argv
    auto config      = wireless_network_config_t{};
    config.name_type = is_bssid(argv[1]) ? wireless_network_name_type_e::BSSID : wireless_network_name_type_e::SSID;
    std::strncpy(config.name, argv[1].c_str(), sizeof(config.name) - 1);
    config.security_type = static_cast<wireless_network_secu_type_e>(std::atol(argv[2].c_str()));
    std::strncpy(config.passwd, argv[3].c_str(), sizeof(config.passwd) - 1);

    int  conn_retry = SSCMA_WIRELESS_NETWORK_CONN_RETRY;
    int  poll_retry = SSCMA_WIRELESS_NETWORK_POLL_RERTY;
    auto ret        = EL_OK;

    // store the config to flash if not invoked in init time
    if (static_resource->is_ready.load()) [[likely]] {
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        if (ret != EL_OK) [[unlikely]]
            goto Reply;
    }

    // check if the network is idle (or ready to connect)
    while (--conn_retry && static_resource->network->status() != NETWORK_IDLE) {
        // check if the network is connected
        while (static_resource->network->status() == NETWORK_JOINED ||
               static_resource->network->status() == NETWORK_CONNECTED) {
            // disconnect the network if joined or connected (an unsubscribe all API is needed)
            ret = static_resource->network->quit();
            if (ret != EL_OK) [[unlikely]] {
                // if failed to disconnect, wait for a while and retry
                el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
                continue;
            }
            // wait for the network to be disconnected
            while (--poll_retry && (static_resource->network->status() == NETWORK_JOINED ||
                                    static_resource->network->status() == NETWORK_CONNECTED))
                el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
            break;  // break if the network is disconnected
        }

        // init again if lost
        if (static_resource->network->status() == NETWORK_LOST) [[unlikely]] {
            static_resource->network->init();
            el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
        }
    }

    // check if the network is idle again, if not, return IO error
    ret = static_resource->network->status() == NETWORK_IDLE ? EL_OK : EL_EIO;
    if (ret != EL_OK) [[unlikely]]
        goto Reply;

    // just return if the network name is empty (a disable API is needed)
    if (argv[1].empty()) [[unlikely]]
        goto Reply;

    // reset retry counter
    conn_retry = SSCMA_WIRELESS_NETWORK_CONN_RETRY;
    poll_retry = SSCMA_WIRELESS_NETWORK_POLL_RERTY;
    // while retrying and the network is not joined
    while (--conn_retry && static_resource->network->status() != NETWORK_JOINED) {
        // try to join to the network
        ret = static_resource->network->join(config.name, config.passwd);
        if (ret != EL_OK) [[unlikely]] {
            // if failed to join, wait for a while and retry
            el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
            continue;
        }
        // wait for the network to be joined
        while (--poll_retry && static_resource->network->status() != NETWORK_JOINED)
            el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
        break;
    }

    // chain setup MQTT server (skip checking if the network is joined)
    {
        auto config = mqtt_server_config_t{};
        auto kv     = el_make_storage_kv_from_type(config);
        if (static_resource->storage->get(kv)) [[likely]]
            set_mqtt_server(std::vector<std::string>{argv[0] + "@MQTTSERVER",
                                                     config.client_id,
                                                     config.address,
                                                     config.username,
                                                     config.password,
                                                     std::to_string(config.use_ssl ? 1 : 0)},
#if CONFIG_EL_DEBUG > 1
                            true
#else
                            false
#endif
            );
    }

Reply:
    if (!has_reply) return;

    auto joined =
      static_resource->network->status() == NETWORK_JOINED || static_resource->network->status() == NETWORK_CONNECTED;
    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"joined\": ",
                                  std::to_string(joined ? 1 : 0),
                                  ", \"config\": ",
                                  wireless_network_config_2_json_str(config),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void get_wireless_network(const std::string& cmd) {
    auto sta    = static_resource->network->status();
    bool joined = sta == NETWORK_JOINED || sta == NETWORK_CONNECTED;
    auto config = wireless_network_config_t{};
    auto ret    = static_resource->storage->get(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"joined\": ",
                                  std::to_string(joined ? 1 : 0),
                                  ", \"config\": ",
                                  wireless_network_config_2_json_str(config),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
