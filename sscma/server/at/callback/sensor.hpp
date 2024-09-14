#pragma once

#include <string>
#include <vector>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

namespace ma::server::callback {

using namespace ma;

void getAvailableSensors(const std::vector<std::string>& args, Transport& transport, Encoder& encoder) {
    MA_ASSERT(args.size() >= 1);
    const auto& cmd = args[0];

    auto& sensors = static_resource->device->getSensors();

    encoder.begin(MA_MSG_TYPE_RESP, MA_OK, cmd);
    encoder.write(sensors);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void configureSensor(const std::vector<std::string>& args,
                     Transport&                      transport,
                     Encoder&                        encoder,
                     bool                            called_by_event = false) {
    MA_ASSERT(args.size() >= 4);
    const auto& cmd       = args[0];
    const auto  sensor_id = std::atoi(args[1].c_str());
    const auto  enable    = std::atoi(args[2].c_str());
    uint8_t     opt_id    = std::atoi(args[3].c_str());
    auto&       sensors   = static_resource->device->getSensors();

    ma_err_t ret = MA_OK;

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

void getSensorStatus(const std::vector<std::string>& args, Transport& transport, Encoder& encoder) {
    MA_ASSERT(args.size() >= 1);
    const auto& cmd = args[0];

    ma_err_t ret     = MA_OK;
    auto&    sensors = static_resource->device->getSensors();

    auto it = std::find_if(sensors.begin(), sensors.end(), [&](const Sensor* s) {
        return s->getID() == static_resource->current_sensor_id;
    });
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
