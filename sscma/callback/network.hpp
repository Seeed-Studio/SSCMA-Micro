#pragma once

#include <cstring>
#include <functional>
#include <string>

#include "shared/network_supervisor.hpp"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::types;
using namespace sscma::utility;

void set_wireless_network(const std::vector<std::string>& argv) {
    // crate config from argv
    auto config      = wireless_network_config_t{};
    config.name_type = is_bssid(argv[1]) ? wireless_network_name_type_e::BSSID : wireless_network_name_type_e::SSID;
    std::strncpy(config.name, argv[1].c_str(), sizeof(config.name) - 1);
    config.security_type = static_cast<wireless_network_secu_type_e>(std::atol(argv[2].c_str()));
    std::strncpy(config.passwd, argv[3].c_str(), sizeof(config.passwd) - 1);

    NetworkSupervisor::get_network_supervisor()->update_wireless_network_config(config);
    auto ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_FAILED;

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": ",
                                  wireless_network_config_2_json_str(config, false),
                                  "}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void get_wireless_network(const std::string& cmd) {
    auto    sta      = static_resource->network->status();
    bool    joined   = sta == NETWORK_JOINED || sta == NETWORK_CONNECTED;
    bool    updated  = NetworkSupervisor::get_network_supervisor()->is_wireless_network_config_updated();
    uint8_t sta_code = joined ? (updated ? 2 : 1) : 0;
    auto    config   = wireless_network_config_t{};
    static_resource->storage->get(el_make_storage_kv_from_type(config));  // discard return error code

    // 0: idle
    // 1: joined + staled
    // 2: joined + updated

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": 0, \"data\": {\"status\": ",
                                  std::to_string(sta_code),
                                  ", \"config\": ",
                                  wireless_network_config_2_json_str(config),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
