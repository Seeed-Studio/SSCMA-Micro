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

#if SSCMA_HAS_NATIVE_NETWORKING == 0
namespace shared_variables {
static int32_t              mqtt_status = 0;
static mqtt_server_config_t mqtt_server_config{};
}  // namespace shared_variables
#endif

void get_mqtt_pubsub(const std::string& cmd, void* caller) {
#if SSCMA_HAS_NATIVE_NETWORKING
    auto config = static_resource->mqtt->get_mqtt_pubsub_config();
#else
    auto config = mqtt_pubsub_config_t{};

    std::snprintf(config.pub_topic,
                  sizeof(config.pub_topic) - 1,
                  SSCMA_MQTT_DESTINATION_FMT "/tx",
                  SSCMA_AT_API_MAJOR_VERSION,
                  shared_variables::mqtt_server_config.client_id);
    config.pub_qos = 0;

    std::snprintf(config.sub_topic,
                  sizeof(config.sub_topic) - 1,
                  SSCMA_MQTT_DESTINATION_FMT "/rx",
                  SSCMA_AT_API_MAJOR_VERSION,
                  shared_variables::mqtt_server_config.client_id);
    config.sub_qos = 0;
#endif

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           cmd,
                           "\", \"code\": 0, \"data\": {\"config\": ",
                           mqtt_pubsub_config_2_json_str(config),
                           "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

void set_mqtt_server(const std::vector<std::string>& argv, void* caller, bool called_by_event = false) {
    auto ret    = EL_OK;
    auto config = get_default_mqtt_server_config(static_resource->device);

    if (argv[1].size() >= sizeof(config.client_id) || argv[2].size() >= sizeof(config.address) ||
        argv[4].size() >= sizeof(config.username) || argv[5].size() >= sizeof(config.password)) [[unlikely]] {
        ret = EL_EINVAL;
        goto Reply;
    }

    if (argv[1].size()) std::strncpy(config.client_id, argv[1].c_str(), sizeof(config.client_id) - 1);

    std::strncpy(config.address, argv[2].c_str(), sizeof(config.address) - 1);

    config.port = std::atoi(argv[3].c_str());

    std::strncpy(config.username, argv[4].c_str(), sizeof(config.username) - 1);
    std::strncpy(config.password, argv[5].c_str(), sizeof(config.password) - 1);

    config.use_ssl = std::atoi(argv[6].c_str()) != 0;

#if SSCMA_HAS_NATIVE_NETWORKING
    ret = static_resource->mqtt->set_mqtt_server_config(config) ? EL_OK : EL_EINVAL;
    if (ret != EL_OK) [[unlikely]]
        goto Reply;
#else
    shared_variables::mqtt_server_config = config;
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
                           mqtt_server_config_2_json_str(config, called_by_event),
                           "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

#if SSCMA_HAS_NATIVE_NETWORKING == 0
void set_mqtt_status(const std::vector<std::string>& argv, void* caller) {
    shared_variables::mqtt_status = std::atoi(argv[1].c_str());

    auto ss{concat_strings("\r{\"type\": 0",
                           ", \"name\": \"",
                           argv[0],
                           "\", \"code\": 0, \"data\": {\"status\": ",
                           std::to_string(shared_variables::mqtt_status),
                           "}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}
#endif

void get_mqtt_server(const std::string& cmd, void* caller) {
#if SSCMA_HAS_NATIVE_NETWORKING
    bool    connected = static_resource->mqtt->is_mqtt_server_connected();
    bool    updated   = static_resource->mqtt->is_mqtt_server_config_synchornized();
    uint8_t sta_code  = connected ? (updated ? 2 : 1) : 0;
    auto    config    = static_resource->mqtt->get_mqtt_server_config();
    // 0: joined
    // 1: connected + staled
    // 2: connected + updated
#else
    uint8_t sta_code = shared_variables::mqtt_status;
    auto    config   = mqtt_server_config_t{};
    static_resource->storage->get(el_make_storage_kv_from_type(config));  // discard return error code
#endif

    auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                           cmd,
                           "\", \"code\": 0, \"data\": {\"status\": ",
                           std::to_string(sta_code),
                           ", \"config\": ",
                           mqtt_server_config_2_json_str(config),
                           "}}\n")};
    static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
