#pragma once

#include <cstring>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::types;
using namespace sscma::utility;

void set_wireless_network(const std::vector<std::string>& argv) {
    // "CMD","NAME","SECURITY_TYPE","PASSWORD"
    auto config = wireless_network_config_t{};
    auto sta    = static_resource->network->status();
    bool joined = sta == NETWORK_JOINED || sta == NETWORK_CONNECTED;

    int retry_cnt = SSCMA_WIRELESS_NETWORK_CONN_RETRY;
    while (--retry_cnt && static_resource->network->status() != NETWORK_IDLE) {
        auto sta = static_resource->network->status();
        if (sta == NETWORK_JOINED || sta == NETWORK_CONNECTED) {
            static_resource->network->quit();
            el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
        }
        static_resource->network->init();
        el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
    }

    auto ret = static_resource->network->status() == NETWORK_IDLE ? EL_OK : EL_EIO;
    if (ret != EL_OK) [[unlikely]]
        goto Reply;

    config.name_type = is_bssid(argv[1]) ? wireless_network_name_type_e::BSSID : wireless_network_name_type_e::SSID;
    std::strncpy(config.name, argv[1].c_str(), sizeof(config.name) - 1);

    config.security_type = static_cast<wireless_network_secu_type_e>(std::atol(argv[2].c_str()));
    std::strncpy(config.passwd, argv[3].c_str(), sizeof(config.passwd) - 1);

    if (static_resource->is_ready.load()) [[likely]] {
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        if (ret != EL_OK) [[unlikely]]
            goto Reply;
    }

    if (argv[1].empty()) goto Reply;

    retry_cnt = SSCMA_WIRELESS_NETWORK_CONN_RETRY;
    while (--retry_cnt && static_resource->network->status() != NETWORK_JOINED) {
        ret = static_resource->network->join(config.name, config.passwd);
        if (ret == EL_OK) break;
        el_sleep(SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS);
    }

Reply:
#if CONFIG_EL_DEBUG == 0
    if (!static_resource->is_ready.load()) return;
#endif

    joined = retry_cnt && static_resource->network->status() == NETWORK_JOINED;
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
