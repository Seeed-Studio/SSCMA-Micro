#pragma once

#include <cstring>
#include <functional>
#include <string>
#include <unordered_set>

#include "shared/network_utility.hpp"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::types;
using namespace sscma::utility;

static void mqtt_recv_cb(char* top, int tlen, char* msg, int mlen) {
    auto config = static_resource->transport->get_mqtt_pubsub_config();
    if (tlen ^ std::strlen(config.sub_topic) || std::strncmp(top, config.sub_topic, tlen) || mlen <= 1) return;
    static_resource->instance->exec(std::string(msg, mlen));
}

void set_mqtt_pubsub(const std::vector<std::string>& argv) {
    // crate config from argv
    auto config = get_default_mqtt_pubsub_config();
    if (argv[1].size()) std::strncpy(config.pub_topic, argv[1].c_str(), sizeof(config.pub_topic) - 1);
    config.pub_qos = std::atoi(argv[2].c_str());
    if (argv[3].size()) std::strncpy(config.sub_topic, argv[3].c_str(), sizeof(config.sub_topic) - 1);
    config.sub_qos = std::atoi(argv[4].c_str());

    auto ret = EL_OK;

    {
        const Guard guard(static_resource->network_config_sync_lock);
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        if (ret == EL_OK) [[likely]]
            ++static_resource->mqtt_pubsub_config_revision;
    }

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": ",
                                  mqtt_pubsub_config_2_json_str(config),
                                  "}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void get_mqtt_pubsub(const std::string& cmd) {
    auto config = get_default_mqtt_pubsub_config();
    static_resource->storage->get(el_make_storage_kv_from_type(config));  // discard return error code

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": 0, \"data\": ",
                                  mqtt_pubsub_config_2_json_str(config),
                                  "}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void set_mqtt_server(const std::vector<std::string>& argv) {
    // crate config from argv
    auto config = get_default_mqtt_server_config();
    if (argv[1].size()) std::strncpy(config.client_id, argv[1].c_str(), sizeof(config.client_id) - 1);
    std::strncpy(config.address, argv[2].c_str(), sizeof(config.address) - 1);
    std::strncpy(config.username, argv[3].c_str(), sizeof(config.username) - 1);
    std::strncpy(config.password, argv[4].c_str(), sizeof(config.password) - 1);
    config.use_ssl = std::atoi(argv[5].c_str()) != 0;  // TODO: driver add SSL config support

    auto ret = EL_OK;

    {
        const Guard guard(static_resource->network_config_sync_lock);
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        if (ret == EL_OK) [[likely]]
            ++static_resource->mqtt_server_config_revision;
    }

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": ",
                                  mqtt_server_config_2_json_str(config, false),
                                  "}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void get_mqtt_server(const std::string& cmd) {
    bool connected = static_resource->network->status() == NETWORK_CONNECTED;
    auto config    = get_default_mqtt_server_config();
    static_resource->storage->get(el_make_storage_kv_from_type(config));  // discard return error code

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": 0, \"data\": {\"connected\": ",
                                  std::to_string(connected ? 1 : 0),
                                  ", \"config\": ",
                                  mqtt_server_config_2_json_str(config),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
