#pragma once

#include <cstdint>
#include <string>

#include "callback/action.hpp"
#include "callback/model.hpp"
#include "callback/mqtt.hpp"
#include "callback/sensor.hpp"
#include "callback/wifi.hpp"
#include "core/data/el_data_storage.hpp"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "definations.hpp"
#include "interface/transport/mqtt.hpp"
#include "interface/wifi.hpp"
#include "static_resource.hpp"

namespace sscma::hooks {

using namespace edgelab;
using namespace edgelab::utility;

using namespace sscma::utility;
using namespace sscma::callback;

void init_algorithm_hook(std::string cmd) {
    if (static_resource->current_algorithm_type != EL_ALGO_TYPE_UNDEFINED) [[likely]]
        set_algorithm(
          cmd + "@ALGO", static_resource->current_algorithm_type, static_cast<void*>(static_resource->serial), true);
}

void init_model_hook(std::string cmd) {
    if (static_resource->current_model_id) [[likely]]
        set_model(cmd + "@MODEL", static_resource->current_model_id, static_cast<void*>(static_resource->serial), true);
}

void init_sensor_hook(std::string cmd) {
    if (static_resource->current_sensor_id) [[likely]]
        set_sensor(
          cmd + "@SENSOR", static_resource->current_sensor_id, true, static_cast<void*>(static_resource->serial), true);
}

void init_action_hook(std::string cmd) {
    if (static_resource->storage->contains(SSCMA_STORAGE_KEY_ACTION)) [[likely]] {
        char action[SSCMA_CMD_MAX_LENGTH]{};
        static_resource->storage->get(el_make_storage_kv(SSCMA_STORAGE_KEY_ACTION, action));
        set_action({cmd + "@ACTION", action}, static_cast<void*>(static_resource->serial), true);
    }
}

void init_wifi_hook(std::string cmd) {
    auto config = wifi_config_t{};
    if (static_resource->storage->get(el_make_storage_kv_from_type(config))) [[likely]]
        set_wifi_network(
          {cmd + "@WIFI", std::string(config.name), std::to_string(config.security_type), std::string(config.passwd)},
          static_cast<void*>(static_resource->serial),
          true);
    static_resource->supervisor->register_supervised_object(static_resource->wifi, 10);
}

void init_mqtt_hook(std::string cmd) {
    auto config = mqtt_server_config_t{};
    if (static_resource->storage->get(el_make_storage_kv_from_type(config))) [[likely]]
        set_mqtt_server({cmd + "@MQTTSERVER",
                         std::string(config.client_id),
                         std::string(config.address),
                         std::to_string(config.port),
                         std::string(config.username),
                         std::string(config.password),
                         std::to_string(config.use_ssl)},
                        static_cast<void*>(static_resource->serial),
                        true);
    static_resource->supervisor->register_supervised_object(static_resource->mqtt, 1000);
}

}  // namespace sscma::hooks
