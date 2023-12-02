#pragma once

#include <cstring>
#include <functional>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::types;
using namespace sscma::utility;

void set_wifi_network(const std::vector<std::string>& argv, void* caller, bool called_by_event = false) {
    // crate config from argv
    auto config      = wifi_config_t{};
    config.name_type = is_bssid(argv[1]) ? wifi_name_type_e::BSSID : wifi_name_type_e::SSID;
    std::strncpy(config.name, argv[1].c_str(), sizeof(config.name) - 1);
    config.security_type = static_cast<wifi_secu_type_e>(std::atol(argv[2].c_str()));
    std::strncpy(config.passwd, argv[3].c_str(), sizeof(config.passwd) - 1);

    auto ret = static_resource->wifi->set_wifi_config(config) ? EL_OK : EL_EINVAL;
    if (!called_by_event && ret == EL_OK) [[likely]]
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_FAILED;

    const auto& ss{concat_strings("\r{\"type\": ",
                                  called_by_event ? "1" : "0",
                                  ", \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": ",
                                  wifi_config_2_json_str(config, called_by_event),
                                  "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_wifi_network(const std::string& cmd, void* caller) {
    bool    joined   = static_resource->wifi->is_wifi_joined();
    bool    updated  = static_resource->wifi->is_wifi_config_synchornized();
    uint8_t sta_code = joined ? (updated ? 2 : 1) : 0;
    auto    in4      = static_resource->wifi->get_in4_info();
    auto    in6      = static_resource->wifi->get_in6_info();
    auto    config   = static_resource->wifi->get_wifi_config();
    static_resource->storage->get(el_make_storage_kv_from_type(config));  // discard return error code

    // 0: idle
    // 1: joined + staled
    // 2: joined + updated

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": 0, \"data\": {\"status\": ",
                                  std::to_string(sta_code),
                                  ", \"in4_info\": ",
                                  in4_info_2_json_str(in4),
                                  ", \"in6_info\": ",
                                  in6_info_2_json_str(in6),
                                  ", \"config\": ",
                                  wifi_config_2_json_str(config),
                                  "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
