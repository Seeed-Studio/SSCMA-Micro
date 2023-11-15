#pragma once

#include <cstring>
#include <functional>
#include <string>

#include "mqtt.hpp"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::types;
using namespace sscma::utility;

void set_wireless_network(
  const std::vector<std::string>&  argv,
  bool                             has_reply      = true,
  bool                             write_to_flash = true,
  std::function<void(std::string)> on_joined_hook = [](std::string caller) { init_mqtt_server_hook(caller); }) {
    // disable network supervisor
    static_resource->enable_network_supervisor.store(false);

    // crate config from argv
    auto config      = wireless_network_config_t{};
    config.name_type = is_bssid(argv[1]) ? wireless_network_name_type_e::BSSID : wireless_network_name_type_e::SSID;
    std::strncpy(config.name, argv[1].c_str(), sizeof(config.name) - 1);
    config.security_type = static_cast<wireless_network_secu_type_e>(std::atol(argv[2].c_str()));
    std::strncpy(config.passwd, argv[3].c_str(), sizeof(config.passwd) - 1);

    int  conn_retry = SSCMA_WIRELESS_NETWORK_CONN_RETRY;
    auto ret        = EL_OK;
    auto sta        = static_resource->network->status();

    // store the config to flash
    if (write_to_flash) {
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        if (ret != EL_OK) [[unlikely]]
            goto Reply;
    }

    // if only store the config to flash, skip the following steps
    if (std::atoi(argv[4].c_str())) goto Reply;

// ensure the network is idle
EnsureIdle:
    sta = static_resource->network->status();  // update status
    if (--conn_retry < 0) [[unlikely]] {
        ret = EL_ETIMOUT;
        goto SyncAndReply;
    }
    switch (sta) {
    case NETWORK_LOST:
        // init again if lost
        static_resource->network->init();
        el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
        goto EnsureIdle;

    case NETWORK_CONNECTED:
        // disconnect if connected (an unsubscribe all API is needed)
        ret = static_resource->network->disconnect();
        // if disconnected, wait for a while and check again
        if (ret == EL_OK) [[likely]]
            el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
        goto EnsureIdle;

    case NETWORK_JOINED:
        // quit if joined
        ret = static_resource->network->quit();
        // if quitted, wait for a while and check again
        if (ret == EL_OK) [[likely]]
            el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
        goto EnsureIdle;

    case NETWORK_IDLE:
        break;

    default:
        goto EnsureIdle;
    }

    // just deinit and return if the network name is empty
    if (argv[1].empty()) [[unlikely]] {
        static_resource->network->deinit();  // Question: is deinit synchronous?
        el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
        sta = static_resource->network->status();
        goto SyncAndReply;
    }

    // reset retry counter
    conn_retry = SSCMA_WIRELESS_NETWORK_CONN_RETRY;
    // try serveral times to join the network
TryJoin:
    sta = static_resource->network->status();  // update status
    if (--conn_retry < 0) [[unlikely]] {
        ret = EL_ETIMOUT;
        goto SyncAndReply;
    }
    switch (sta) {
    case NETWORK_LOST:
    case NETWORK_CONNECTED:
        ret = EL_EIO;
        goto SyncAndReply;

    case NETWORK_IDLE:
        // try to join to the network
        ret = static_resource->network->join(config.name, config.passwd);
        // if quitted, wait for a while and check again
        if (ret == EL_OK) [[likely]]
            el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
        goto TryJoin;

    case NETWORK_JOINED:
        break;

    default:
        goto TryJoin;
    }

    // sync status before hook functions
    static_resource->target_network_status = sta;

    // call hook function
    if (on_joined_hook) on_joined_hook(argv[0]);

    // never sync status after hook functions
    goto Reply;

SyncAndReply:
    // sync status
    static_resource->target_network_status = sta;

Reply:
    // enable network supervisor
    static_resource->enable_network_supervisor.store(true);

    if (!has_reply) return;

    bool        joined = sta == NETWORK_JOINED || sta == NETWORK_CONNECTED;
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

void init_wireless_network_hook(std::string cmd) {
#if CONFIG_EL_DEBUG > 1
    bool has_reply = true;
#else
    bool has_reply = false;
#endif
    auto config = wireless_network_config_t{};
    auto kv     = el_make_storage_kv_from_type(config);
    if (static_resource->storage->get(kv)) [[likely]]
        set_wireless_network(
          {cmd + "@WIFI", config.name, std::to_string(config.security_type), config.passwd, "0"}, has_reply, false);
}

}  // namespace sscma::callback
