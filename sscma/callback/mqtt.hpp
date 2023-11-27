#pragma once

#include <cstring>
#include <functional>
#include <string>
#include <unordered_set>

#include "shared/network_supervisor.hpp"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::types;
using namespace sscma::utility;

void set_mqtt_pubsub(const std::vector<std::string>& argv, void* caller) {
    // crate config from argv
    auto config = get_default_mqtt_pubsub_config(static_resource->device);
    if (argv[1].size()) std::strncpy(config.pub_topic, argv[1].c_str(), sizeof(config.pub_topic) - 1);
    config.pub_qos = std::atoi(argv[2].c_str());
    if (argv[3].size()) std::strncpy(config.sub_topic, argv[3].c_str(), sizeof(config.sub_topic) - 1);
    config.sub_qos = std::atoi(argv[4].c_str());

    NetworkSupervisor::get_network_supervisor()->update_mqtt_pubsub_config(config);
    auto ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_FAILED;

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": ",
                                  mqtt_pubsub_config_2_json_str(config),
                                  "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_mqtt_pubsub(const std::string& cmd, void* caller) {
    bool    updated  = NetworkSupervisor::get_network_supervisor()->is_mqtt_pubsub_config_updated();
    uint8_t sta_code = updated ? 2 : 1;
    auto    config   = get_default_mqtt_pubsub_config(static_resource->device);
    static_resource->storage->get(el_make_storage_kv_from_type(config));  // discard return error code

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": 0, \"data\": {\"status\": ",
                                  std::to_string(sta_code),
                                  ", \"config\": ",
                                  mqtt_pubsub_config_2_json_str(config),
                                  "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void set_mqtt_server(const std::vector<std::string>& argv, void* caller) {
    // crate config from argv
    auto config = get_default_mqtt_server_config(static_resource->device);
    if (argv[1].size()) std::strncpy(config.client_id, argv[1].c_str(), sizeof(config.client_id) - 1);
    std::strncpy(config.address, argv[2].c_str(), sizeof(config.address) - 1);
    config.port = std::atoi(argv[3].c_str());
    std::strncpy(config.username, argv[4].c_str(), sizeof(config.username) - 1);
    std::strncpy(config.password, argv[5].c_str(), sizeof(config.password) - 1);
    config.use_ssl = std::atoi(argv[6].c_str()) != 0;  // TODO: driver add SSL config support

    NetworkSupervisor::get_network_supervisor()->update_mqtt_server_config(config);
    auto ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_FAILED;

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": ",
                                  mqtt_server_config_2_json_str(config, false),
                                  "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void get_mqtt_server(const std::string& cmd, void* caller) {
    bool    connected = static_resource->network->status() == NETWORK_CONNECTED;
    bool    updated   = NetworkSupervisor::get_network_supervisor()->is_mqtt_server_config_updated();
    uint8_t sta_code  = connected ? (updated ? 2 : 1) : 0;
    auto    config    = get_default_mqtt_server_config(static_resource->device);
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
