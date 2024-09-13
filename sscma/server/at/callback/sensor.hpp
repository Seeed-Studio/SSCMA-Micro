#pragma once

#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

namespace ma::server::callback {

using namespace ma;

void get_available_sensors(const std::string& cmd, Transport& transport, Encoder& encoder) {
    auto& sensors = static_resource->device->getSensors();
    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd);
    encoder.write(sensors);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void set_sensor(const std::string& cmd,
                size_t             sensor_id,
                bool               enable,
                size_t             opt_id,
                Transport&         transport,
                Encoder&           encoder,
                bool               called_by_event = false) {
    ma_err_t ret     = MA_OK;
    auto&    sensors = static_resource->device->getSensors();

    auto it = std::find_if(sensors.begin(), sensors.end(), [&](const Sensor* s) { return s->getID() == sensor_id; });
    if (it == sensors.end()) {
        ret = MA_ENOENT;
        goto exit;
    }

    switch ((*it)->getType()) {
    case ma::Sensor::Type::kCamera: {
        auto* camera = static_cast<Camera*>(*it);
        if (enable) {
            if (*camera) {
                camera->deInit();
            }
            ret = camera->init(opt_id);
        } else {
            camera->deInit();
        }
    } break;

    default:
        ret = MA_ENOENT;
        break;
    }

    if (ret != MA_OK) {
        goto exit;
    }

    if (static_resource->current_sensor_id != sensor_id) {
        static_resource->current_sensor_id = sensor_id;
        if (!called_by_event) {
            MA_STORAGE_SET_POD(
              ret, static_resource->device->getStorage(), MA_STORAGE_KEY_SENSOR_ID, static_resource->current_sensor_id);
        }
    }

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, cmd);
    if (it != sensors.end()) {
        encoder.write(*it);
    }
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void get_sensor_info(const std::string& cmd, Transport& transport, Encoder& encoder) {
    ma_err_t ret     = MA_OK;
    auto&    sensors = static_resource->device->getSensors();

    auto it = std::find_if(
      sensors.begin(), sensors.end(), [&](const Sensor* s) { return s->getID() == static_resource->current_sensor_id; });
    if (it == sensors.end()) {
        ret = MA_ENOENT;
    }

    encoder.begin(MA_MSG_TYPE_RESP, ret, cmd);
    if (it != sensors.end()) {
        encoder.write(*it);
    }
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback
