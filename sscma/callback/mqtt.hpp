#pragma once

#include <cstring>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::types;
using namespace sscma::utility;

static void mqtt_recv_cb(char* top, int tlen, char* msg, int mlen) {
    const auto& config_topic = static_resource->current_mqtt_pubsub_config.sub_topic;
    if (tlen ^ std::strlen(config_topic) || std::strncmp(top, config_topic, tlen) || mlen <= 1) return;
    static_resource->instance->exec(std::move(std::string(msg, mlen)));
}

void set_mqtt_server(const std::vector<std::string>& argv) {
    // "CMD","CLIENT_ID","ADDRESS","USERNAME","PASSWORD","USE_SSL"
    auto config = mqtt_server_config_t{};
    if (argv[1].empty())
        std::snprintf(config.client_id, sizeof(config.client_id) - 1, "%ld", static_resource->device->get_device_id());
    else
        std::strncpy(config.client_id, argv[1].c_str(), sizeof(config.client_id) - 1);
    std::strncpy(config.address, argv[2].c_str(), sizeof(config.address) - 1);
    std::strncpy(config.username, argv[3].c_str(), sizeof(config.username) - 1);
    std::strncpy(config.password, argv[4].c_str(), sizeof(config.password) - 1);
    config.use_ssl = std::atoi(argv[5].c_str()) != 0;  // TODO: driver add SSL config support

    int  retry_cnt = SSCMA_MQTT_CONN_RETRY;
    bool connected = static_resource->network->status() == NETWORK_CONNECTED;
    auto ret       = EL_OK;

    if (static_resource->is_ready.load()) [[likely]] {
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        if (ret != EL_OK) [[unlikely]]
            goto Reply;
    }

    if (argv[2].empty())  // TODO: driver add disconnect support
        goto Reply;

    while (--retry_cnt && static_resource->network->status() != NETWORK_CONNECTED) {
        ret = static_resource->network->connect(
          config.address, config.username, config.password, sscma::callback::mqtt_recv_cb);
        if (ret == EL_OK) break;
        el_sleep(SSCMA_MQTT_CONN_DELAY_MS);
    }

    // set publish/subscribe topic
    static_resource->transport->set_mqtt_config(static_resource->current_mqtt_pubsub_config);
    ret =
      static_resource->network->subscribe(static_resource->current_mqtt_pubsub_config.sub_topic,
                                          static_cast<mqtt_qos_t>(static_resource->current_mqtt_pubsub_config.sub_qos));
    static_resource->transport->emit_mqtt_discover();

Reply:
#if CONFIG_EL_DEBUG == 0
    if (!static_resource->is_ready.load()) return;
#endif
    connected = retry_cnt && static_resource->network->status() == NETWORK_CONNECTED;
    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  argv[0],
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"connected\": ",
                                  std::to_string(connected ? 1 : 0),
                                  ", \"config\": ",
                                  mqtt_server_config_2_json_str(config),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void get_mqtt_server(const std::string& cmd) {
    bool connected = static_resource->network->status() == NETWORK_CONNECTED;
    auto config    = mqtt_server_config_t{};
    auto ret       = static_resource->storage->get(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"connected\": ",
                                  std::to_string(connected ? 1 : 0),
                                  ", \"config\": ",
                                  mqtt_server_config_2_json_str(config),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void set_mqtt_pubsub(const std::vector<std::string>& argv) {
    // "CMD","PUB_TOPIC","PUB_QOS","SUB_TOPIC","SUB_QOS"
    auto config = mqtt_pubsub_config_t{};
    std::strncpy(config.pub_topic, argv[1].c_str(), sizeof(config.pub_topic) - 1);
    config.pub_qos = std::atoi(argv[2].c_str());
    std::strncpy(config.sub_topic, argv[3].c_str(), sizeof(config.sub_topic) - 1);
    config.sub_qos = std::atoi(argv[4].c_str());

    auto ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
    if (ret != EL_OK) [[unlikely]]
        goto Reply;

    static_resource->current_mqtt_pubsub_config = config;
    static_resource->transport->set_mqtt_config(config);

    ret = static_resource->network->subscribe(config.sub_topic, static_cast<mqtt_qos_t>(config.sub_qos));

Reply:
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
    auto config = static_resource->current_mqtt_pubsub_config;
    auto ret    = static_resource->storage->get(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": ",
                                  mqtt_pubsub_config_2_json_str(config),
                                  "}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
