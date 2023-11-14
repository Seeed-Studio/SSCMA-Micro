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
    auto config = static_resource->transport->get_mqtt_config();
    if (tlen ^ std::strlen(config.sub_topic) || std::strncmp(top, config.sub_topic, tlen) || mlen <= 1) return;
    static_resource->instance->exec(std::move(std::string(msg, mlen)));
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

void set_mqtt_pubsub(const std::vector<std::string>& argv, bool has_reply = true) {
    // crate config from argv
    auto config = mqtt_pubsub_config_t{};
    std::strncpy(config.pub_topic, argv[1].c_str(), sizeof(config.pub_topic) - 1);
    config.pub_qos = std::atoi(argv[2].c_str());
    std::strncpy(config.sub_topic, argv[3].c_str(), sizeof(config.sub_topic) - 1);
    config.sub_qos = std::atoi(argv[4].c_str());

    auto ret = EL_OK;

    // store the config to flash if not invoked in init time
    if (static_resource->is_ready.load()) [[likely]] {
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        if (ret != EL_OK) [[unlikely]]
            goto Reply;
    }

    // unsubscribe old topic if exist (a unsubscribe all API is needed)
    if (std::strlen(static_resource->current_mqtt_pubsub_config.sub_topic))
        static_resource->network->unsubscribe(static_resource->current_mqtt_pubsub_config.sub_topic);

    // update current config
    static_resource->current_mqtt_pubsub_config = config;
    // subscribe new topic
    ret = static_resource->network->subscribe(config.sub_topic, static_cast<mqtt_qos_t>(config.sub_qos));
    // set MQTT config for transport
    static_resource->transport->set_mqtt_config(config);
    // publish the pubsub config to MQTT discover topic
    static_resource->transport->emit_mqtt_discover();

Reply:
    if (!has_reply) return;

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

void set_mqtt_server(const std::vector<std::string>& argv, bool has_reply = true) {
    // crate config from argv
    auto config = mqtt_server_config_t{};
    if (argv[1].empty())  // if the client id is empty, generate one
        std::snprintf(config.client_id,
                      sizeof(config.client_id) - 1,
                      SSCMA_MQTT_DEVICE_ID_FMT,
                      VENDOR_PREFIX,
                      VENDOR_CHIP_NAME,
                      static_resource->device->get_device_id());
    else
        std::strncpy(config.client_id, argv[1].c_str(), sizeof(config.client_id) - 1);
    std::strncpy(config.address, argv[2].c_str(), sizeof(config.address) - 1);
    std::strncpy(config.username, argv[3].c_str(), sizeof(config.username) - 1);
    std::strncpy(config.password, argv[4].c_str(), sizeof(config.password) - 1);
    config.use_ssl = std::atoi(argv[5].c_str()) != 0;  // TODO: driver add SSL config support

    int  conn_retry = SSCMA_MQTT_CONN_RETRY;
    int  poll_retry = SSCMA_MQTT_POLL_RETRY;
    auto ret        = EL_OK;

    // store the config to flash if not invoked in init time
    if (static_resource->is_ready.load()) [[likely]] {
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        if (ret != EL_OK) [[unlikely]]
            goto Reply;
    }

    // if the MQTT server is connected, disconnect it
    // while (--conn_retry && static_resource->network->status() == NETWORK_CONNECTED) {
    //     // TODO: driver add disconnect API
    //     ret = static_resource->network->disconnect();
    //     if (ret != EL_OK) [[unlikely]] {
    //         el_sleep(SSCMA_MQTT_CONN_DELAY_MS);
    //         continue;
    //     }
    //     while (--poll_retry && static_resource->network->status() == NETWORK_CONNECTED)
    //         el_sleep(SSCMA_MQTT_CONN_DELAY_MS);
    //     break;
    // }

    // check if the MQTT server is disconnected again, if not, return IO error
    // ret = static_resource->network->status() == NETWORK_JOINED ? EL_OK : EL_EIO;
    // if (ret != EL_OK) [[unlikely]]
    //     goto Reply;

    // just return if the server address is empty (a disable API is needed)
    if (argv[2].empty()) goto Reply;

    // reset retry counter
    conn_retry = SSCMA_MQTT_CONN_RETRY;
    poll_retry = SSCMA_MQTT_POLL_RETRY;
    // if the MQTT server is not connected, try connect
    while (--conn_retry && static_resource->network->status() != NETWORK_CONNECTED) {
        // try connect to MQTT server
        ret = static_resource->network->connect(
          config.address, config.username, config.password, sscma::callback::mqtt_recv_cb);
        if (ret != EL_OK) [[unlikely]] {
            // if failed to connect, wait for a while and retry
            el_sleep(SSCMA_MQTT_CONN_DELAY_MS);
            continue;
        }
        // wait for the MQTT server to be connected
        while (--poll_retry && static_resource->network->status() != NETWORK_CONNECTED)
            el_sleep(SSCMA_MQTT_CONN_DELAY_MS);
        break;
    }

    // chain setup MQTT server publish and subscribe topic (skip checking if the MQTT server is connected)
    {
        auto config = static_resource->current_mqtt_pubsub_config;
        set_mqtt_pubsub(std::vector<std::string>{argv[0] + "@MQTTPUBSUB",
                                                 config.pub_topic,
                                                 std::to_string(config.pub_qos),
                                                 config.sub_topic,
                                                 std::to_string(config.sub_qos)},
#if CONFIG_EL_DEBUG > 1
                        true
#else
                        false
#endif
        );
    }

Reply:
    if (!has_reply) return;

    auto        connected = static_resource->network->status() == NETWORK_CONNECTED;
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

}  // namespace sscma::callback
