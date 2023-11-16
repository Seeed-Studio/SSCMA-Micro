#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "callback/action.hpp"
#include "callback/model.hpp"
#include "callback/mqtt.hpp"
#include "callback/network.hpp"
#include "callback/sensor.hpp"
#include "core/data/el_data_storage.hpp"
#include "core/el_types.h"

namespace sscma::hooks {

using namespace edgelab;
using namespace edgelab::utility;

using namespace sscma::callback;

void init_model_hook(std::string cmd) {
    if (static_resource->current_model_id) [[likely]]
        set_model(cmd + "@MODEL", static_resource->current_model_id, true);
}

void init_sensor_hook(std::string cmd) {
    if (static_resource->current_sensor_id) [[likely]]
        set_sensor(cmd + "@SENSOR", static_resource->current_sensor_id, true, true);
}

void init_action_hook(std::string cmd) {
    if (static_resource->storage->contains(SSCMA_STORAGE_KEY_ACTION)) [[likely]] {
        char action[CONFIG_SSCMA_CMD_MAX_LENGTH]{};
        static_resource->storage->get(el_make_storage_kv(SSCMA_STORAGE_KEY_ACTION, action));
        set_action(std::vector<std::string>{cmd + "@ACTION", action}, true);
    }
}

void init_mqtt_pubsub_hook(std::string cmd) {
    auto config = static_resource->current_mqtt_pubsub_config;
    set_mqtt_pubsub({cmd + "@MQTTPUBSUB",
                     config.pub_topic,
                     std::to_string(config.pub_qos),
                     config.sub_topic,
                     std::to_string(config.sub_qos)},
                    true,
                    [](std::string) { static_resource->transport->emit_mqtt_discover(); });
}

void init_mqtt_server_hook(std::string cmd) {
    auto config = mqtt_server_config_t{};
    auto kv     = el_make_storage_kv_from_type(config);
    if (static_resource->storage->get(kv)) [[likely]]
        set_mqtt_server({cmd + "@MQTTSERVER",
                         config.client_id,
                         config.address,
                         config.username,
                         config.password,
                         std::to_string(config.use_ssl ? 1 : 0)},
                        true,
                        init_mqtt_pubsub_hook);
}

void init_wireless_network_hook(std::string cmd) {
    auto config = wireless_network_config_t{};
    auto kv     = el_make_storage_kv_from_type(config);
    if (static_resource->storage->get(kv)) [[likely]]
        set_wireless_network({cmd + "@WIFI", config.name, std::to_string(config.security_type), config.passwd},
                             true,
                             init_mqtt_server_hook);
}

void on_network_status_change_hook(el_net_sta_t current_status) {
    if (!static_resource->enable_network_supervisor.load()) return;

    auto target_network_status = static_resource->target_network_status;
    if (target_network_status == current_status) return;

    std::string caller{"SUPERVISOR"};

    using namespace sscma::callback;
    switch (current_status) {
    case NETWORK_LOST:
    case NETWORK_IDLE:
        return init_wireless_network_hook(caller);
    case NETWORK_JOINED:
        return init_mqtt_server_hook(caller);
    case NETWORK_CONNECTED:
        return init_mqtt_pubsub_hook(caller);
    }
}

}  // namespace sscma::hooks
