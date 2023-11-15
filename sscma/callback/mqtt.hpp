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

static void mqtt_recv_cb(char* top, int tlen, char* msg, int mlen) {
    auto config = static_resource->transport->get_mqtt_config();
    if (tlen ^ std::strlen(config.sub_topic) || std::strncmp(top, config.sub_topic, tlen) || mlen <= 1) return;
    static_resource->instance->exec(std::string(msg, mlen));
}

void set_mqtt_pubsub(const std::vector<std::string>& argv, bool has_reply = true, bool write_to_flash = true) {
    // crate config from argv
    auto config = mqtt_pubsub_config_t{};
    std::strncpy(config.pub_topic, argv[1].c_str(), sizeof(config.pub_topic) - 1);
    config.pub_qos = std::atoi(argv[2].c_str());
    std::strncpy(config.sub_topic, argv[3].c_str(), sizeof(config.sub_topic) - 1);
    config.sub_qos = std::atoi(argv[4].c_str());

    auto ret = EL_OK;

    // store the config to flash if not invoked in init time
    if (write_to_flash) {
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        if (ret != EL_OK) [[unlikely]]
            goto Reply;
    }

    // if only store the config to flash, skip the following steps
    if (std::atoi(argv[5].c_str())) goto Reply;

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

void init_mqtt_pubsub_hook(std::string cmd) {
#if CONFIG_EL_DEBUG > 1
    bool has_reply = true;
#else
    bool has_reply = false;
#endif
    auto config = static_resource->current_mqtt_pubsub_config;
    set_mqtt_pubsub({cmd + "@MQTTPUBSUB",
                     config.pub_topic,
                     std::to_string(config.pub_qos),
                     config.sub_topic,
                     std::to_string(config.sub_qos),
                     "0"},
                    has_reply,
                    false);
}

void set_mqtt_server(
  const std::vector<std::string>&  argv,
  bool                             has_reply         = true,
  bool                             write_to_flash    = true,
  std::function<void(std::string)> on_connected_hook = [](std::string caller) { init_mqtt_pubsub_hook(caller); }) {
    // disable network supervisor
    static_resource->enable_network_supervisor.store(false);

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
    auto ret        = EL_OK;
    auto sta        = static_resource->network->status();

    // store the config to flash if not invoked in init time
    if (write_to_flash) {
        ret = static_resource->storage->emplace(el_make_storage_kv_from_type(config)) ? EL_OK : EL_EIO;
        if (ret != EL_OK) [[unlikely]]
            goto Reply;
    }

    // if only store the config to flash, skip the following steps
    if (std::atoi(argv[6].c_str())) goto Reply;

// ensure the network is joined
EnsureJoined:
    sta = static_resource->network->status();  // update status
    if (--conn_retry < 0) [[unlikely]] {
        ret = EL_ETIMOUT;
        goto SyncAndReply;
    }
    switch (sta) {
    case NETWORK_LOST:
    case NETWORK_IDLE:
        ret = EL_EPERM;
        goto SyncAndReply;

    case NETWORK_CONNECTED:
        // try to disconnect
        ret = static_resource->network->disconnect();
        // if disconnected, wait for a while and check again
        if (ret == EL_OK) [[likely]]
            el_sleep(SSCMA_MQTT_CONN_DELAY_MS);
        goto EnsureJoined;

    case NETWORK_JOINED:
        break;

    default:
        goto EnsureJoined;
    }

    // just return if the server address is empty (a disable API is needed)
    if (argv[2].empty()) [[unlikely]] {
        sta = static_resource->network->status();
        goto SyncAndReply;
    }

    // reset retry counter
    conn_retry = SSCMA_MQTT_CONN_RETRY;
// try serveral times to connect to MQTT server
TryConnect:
    sta = static_resource->network->status();  // update status
    if (--conn_retry < 0) [[unlikely]] {
        ret = EL_ETIMOUT;
        goto SyncAndReply;
    }
    switch (sta) {
    case NETWORK_LOST:
    case NETWORK_IDLE:
        ret = EL_EIO;
        goto SyncAndReply;

    case NETWORK_JOINED:
        // try to connect
        ret = static_resource->network->connect(
          config.address, config.username, config.password, sscma::callback::mqtt_recv_cb);
        // if disconnected, wait for a while and check again
        if (ret == EL_OK) [[likely]]
            el_sleep(SSCMA_MQTT_CONN_DELAY_MS);
        goto TryConnect;

    case NETWORK_CONNECTED:
        break;

    default:
        goto TryConnect;
    }

    // sync status before hook functions
    static_resource->target_network_status = sta;

    // call hook function
    if (on_connected_hook) on_connected_hook(argv[0]);

    // never sync status after hook functions
    goto Reply;

SyncAndReply:
    // sync status
    static_resource->target_network_status = sta;

Reply:
    // enable network supervisor
    static_resource->enable_network_supervisor.store(true);

    if (!has_reply) return;

    bool        connected = static_resource->network->status() == NETWORK_CONNECTED;
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

void init_mqtt_server_hook(std::string cmd) {
#if CONFIG_EL_DEBUG > 1
    bool has_reply = true;
#else
    bool has_reply = false;
#endif
    auto config = mqtt_server_config_t{};
    auto kv     = el_make_storage_kv_from_type(config);
    if (static_resource->storage->get(kv)) [[likely]]
        set_mqtt_server({cmd + "@MQTTSERVER",
                         config.client_id,
                         config.address,
                         config.username,
                         config.password,
                         std::to_string(config.use_ssl ? 1 : 0),
                         "0"},
                        has_reply,
                        false);
}

}  // namespace sscma::callback
