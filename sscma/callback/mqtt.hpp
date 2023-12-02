#pragma once

#include <cstring>
#include <functional>
#include <string>
#include <unordered_set>

#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::types;
using namespace sscma::utility;

void get_mqtt_pubsub(const std::string& cmd, void* caller) {
    auto config = static_resource->mqtt->get_mqtt_pubsub_config();

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": 0, \"data\": {\"config\": ",
                                  mqtt_pubsub_config_2_json_str(config),
                                  "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void set_mqtt_server(const std::vector<std::string>& argv, void* caller, bool called_by_event = false) {
    // crate config from argv
    auto config = get_default_mqtt_server_config(static_resource->device);
    if (argv[1].size()) std::strncpy(config.client_id, argv[1].c_str(), sizeof(config.client_id) - 1);
    std::strncpy(config.address, argv[2].c_str(), sizeof(config.address) - 1);
    config.port = std::atoi(argv[3].c_str());
    std::strncpy(config.username, argv[4].c_str(), sizeof(config.username) - 1);
    std::strncpy(config.password, argv[5].c_str(), sizeof(config.password) - 1);
    config.use_ssl = std::atoi(argv[6].c_str()) != 0;  // TODO: driver add SSL config support

    auto ret = static_resource->mqtt->set_mqtt_server_config(config) ? EL_OK : EL_EINVAL;
    if (!called_by_event && ret == EL_OK) [[likely]]
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_FAILED;

    const auto& ss{concat_strings("\r{\"type\": ",
                                  called_by_event ? "1" : "0",
                                  ", \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": ",
                                  mqtt_server_config_2_json_str(config, called_by_event),
                                  "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_mqtt_server(const std::string& cmd, void* caller) {
    bool    connected = static_resource->mqtt->is_mqtt_server_connected();
    bool    updated   = static_resource->mqtt->is_mqtt_server_config_synchornized();
    uint8_t sta_code  = connected ? (updated ? 2 : 1) : 0;
    auto    config    = static_resource->mqtt->get_mqtt_server_config();
    static_resource->storage->get(el_make_storage_kv_from_type(config));  // discard return error code

    // 0: joined
    // 1: connected + staled
    // 2: connected + updated

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": 0, \"data\": {\"status\": ",
                                  std::to_string(sta_code),
                                  ", \"config\": ",
                                  mqtt_server_config_2_json_str(config),
                                  "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
