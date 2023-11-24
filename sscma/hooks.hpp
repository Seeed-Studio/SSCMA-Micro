#pragma once

#include <cstdint>
#include <string>

#include "callback/action.hpp"
#include "callback/model.hpp"
#include "callback/mqtt.hpp"
#include "callback/network.hpp"
#include "callback/sensor.hpp"
#include "callback/shared/network_supervisor.hpp"
#include "core/data/el_data_storage.hpp"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "definations.hpp"
#include "static_resource.hpp"

namespace sscma::hooks {

using namespace edgelab;
using namespace edgelab::utility;

using namespace sscma::utility;
using namespace sscma::callback;
using namespace sscma::extension;

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
        char action[SSCMA_CMD_MAX_LENGTH]{};
        static_resource->storage->get(el_make_storage_kv(SSCMA_STORAGE_KEY_ACTION, action));
        set_action({cmd + "@ACTION", action}, true);
    }
}

void init_network_supervisor_task_hook() { NetworkSupervisor::get_network_supervisor()->start(); }

}  // namespace sscma::hooks
