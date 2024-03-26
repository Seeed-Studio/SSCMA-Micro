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

#if SSCMA_HAS_NATIVE_NETWORKING == 0
namespace shared_variables {
static std::string wifi_ver    = "";
static int32_t     wifi_status = 0;
static in4_info_t  in4_info{};
static in6_info_t  in6_info{};
}  // namespace shared_variables
#endif

void set_wifi_ver(const std::vector<std::string>& argv, void* caller) {
    shared_variables::wifi_ver = argv[1];

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           argv[0],
                           "\", \"code\": 0, \"data\": {\"ver\": \"",
                           shared_variables::wifi_ver,
                           "\"}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_wifi_ver(const std::string& cmd, void* caller) {
    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           cmd,
                           "\", \"code\": 0, \"data\": {\"ver\": \"",
                           shared_variables::wifi_ver,
                           "\"}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void set_wifi_network(const std::vector<std::string>& argv, void* caller, bool called_by_event = false) {
    auto ret    = EL_OK;
    auto config = wifi_sta_cfg_t{};

    if (argv[1].size() >= sizeof(config.name) || argv[3].size() >= sizeof(config.passwd)) [[unlikely]] {
        ret = EL_EINVAL;
        goto Reply;
    }

    config.name_type = is_bssid(argv[1]) ? wifi_name_type_e::BSSID : wifi_name_type_e::SSID;
    std::strncpy(config.name, argv[1].c_str(), sizeof(config.name) - 1);

    config.security_type = static_cast<wifi_secu_type_e>(std::atol(argv[2].c_str()));
    std::strncpy(config.passwd, argv[3].c_str(), sizeof(config.passwd) - 1);

#if SSCMA_HAS_NATIVE_NETWORKING
    ret = static_resource->wifi->set_wifi_config(config) ? EL_OK : EL_EINVAL;
    if (ret != EL_OK) [[unlikely]]
        goto Reply;
#endif

    if (!called_by_event) [[likely]]
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_FAILED;

Reply:
    auto ss{concat_strings("\r{\"type\": ",
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

#if SSCMA_HAS_NATIVE_NETWORKING == 0
void set_wifi_status(const std::vector<std::string>& argv, void* caller) {
    shared_variables::wifi_status = std::atoi(argv[1].c_str());

    auto ss{concat_strings("\r{\"type\": 0",
                           ", \"name\": \"",
                           argv[0],
                           "\", \"code\": 0, \"data\": {\"status\": ",
                           std::to_string(shared_variables::wifi_status),
                           "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void set_wifi_in4_info(const std::vector<std::string>& argv, void* caller) {
    shared_variables::in4_info.ip      = ipv4_addr_t::from_str(argv[1]);
    shared_variables::in4_info.netmask = ipv4_addr_t::from_str(argv[2]);
    shared_variables::in4_info.gateway = ipv4_addr_t::from_str(argv[3]);

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           argv[0],
                           "\", \"code\": 0, \"data\": {\"in4_info\": ",
                           in4_info_2_json_str(shared_variables::in4_info),
                           "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void set_wifi_in6_info(const std::vector<std::string>& argv, void* caller) {
    shared_variables::in6_info.ip = ipv6_addr_t::from_str(argv[1]);

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           argv[0],
                           "\", \"code\": 0, \"data\": {\"in6_info\": ",
                           in6_info_2_json_str(shared_variables::in6_info),
                           "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}
#endif

void get_wifi_network(const std::string& cmd, void* caller) {
#if SSCMA_HAS_NATIVE_NETWORKING
    bool    joined   = static_resource->wifi->is_wifi_joined();
    bool    updated  = static_resource->wifi->is_wifi_config_synchornized();
    uint8_t sta_code = joined ? (updated ? 2 : 1) : 0;
    auto    in4      = static_resource->wifi->get_in4_info();
    auto    in6      = static_resource->wifi->get_in6_info();
    auto    config   = static_resource->wifi->get_wifi_config();
// 0: idle
// 1: joined + staled
// 2: joined + updated
#else
    uint8_t sta_code = shared_variables::wifi_status;
    auto    in4      = shared_variables::in4_info;
    auto    in6      = shared_variables::in6_info;
    auto    config   = wifi_sta_cfg_t{};
#endif
    static_resource->storage->get(el_make_storage_kv_from_type(config));  // discard return error code

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
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
