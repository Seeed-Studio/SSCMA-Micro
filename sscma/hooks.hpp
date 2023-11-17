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
#include "core/el_config_internal.h"
#include "core/el_types.h"
#include "definations.hpp"

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

void network_supervisor_hook() {
    if (!static_resource->enable_network_supervisor.load()) return;

    EL_LOGI("network_supervisor: Checking network status...");

    static_resource->executor->add_task([](const std::atomic<bool>&) {
        static_resource->enable_network_supervisor.store(false);

        auto target_status  = static_resource->target_network_status;
        auto current_status = static_resource->network->status();
        if (target_status == current_status) return;

        EL_LOGI("network_supervisor: Unexpected network status, trying to recover...");

        std::string caller{"SUPERVISOR"};
        switch (current_status) {
        case NETWORK_LOST:
        case NETWORK_IDLE:
            init_wireless_network_hook(caller);
            break;
        case NETWORK_JOINED:
            init_mqtt_server_hook(caller);
            break;
        case NETWORK_CONNECTED:
        default:
            break;
        }

        static_resource->enable_network_supervisor.store(true);
    });
}

void init_network_supervisor_hook(void*) {
Loop:
    EL_LOGI("network_supervisor: Calling network supervisor hook...");

    network_supervisor_hook();
    el_sleep(SSCMA_NETWORK_SUPERVISOR_POLL_DELAY);

    goto Loop;
}

void init_network_supervisor_task_hook() {
    static_resource->enable_network_supervisor.store(true);
    xTaskCreate(init_network_supervisor_hook,
                SSCMA_NETWORK_SUPERVISOR_NAME,
                SSCMA_NETWORK_SUPERVISOR_STACK_SIZE,
                nullptr,
                SSCMA_NETWORK_SUPERVISOR_PRIO,
                nullptr);
}

}  // namespace sscma::hooks
