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
    const auto&  cmd       = args[0];
    const size_t sensor_id = std::atoi(args[1].c_str());
    const size_t enable    = std::atoi(args[2].c_str());
    const size_t opt_id    = std::atoi(args[3].c_str());
    auto&        sensors   = static_resource->device->getSensors();

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

    static_resource->current_sensor_id = sensor_id;

    if (!called_by_event) {
        MA_STORAGE_SET_POD(
          ret, static_resource->device->getStorage(), MA_STORAGE_KEY_SENSOR_ID, static_resource->current_sensor_id);
        MA_STORAGE_SET_POD(ret, static_resource->device->getStorage(), MA_STORAGE_KEY_SENSOR_OPT_ID, opt_id);
    }

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, cmd);
    if (it != sensors.end()) {
        encoder.write(*it, (*it)->currentPresetIdx());
    }
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void initDefaultSensor(Encoder& encoder) {
    if (static_resource->device->getTransports().empty()) {
        MA_LOGD(MA_TAG, "No transport available");
        return;
    }
    auto& transport = static_resource->device->getTransports().front();
    if (!transport || !*transport) {
        MA_LOGD(MA_TAG, "Transport not available");
        return;
    }

    size_t sensor_id = 0;
    size_t opt_id    = 0;

    MA_STORAGE_GET_POD(static_resource->device->getStorage(), MA_STORAGE_KEY_SENSOR_ID, sensor_id, 0);
    MA_STORAGE_GET_POD(static_resource->device->getStorage(), MA_STORAGE_KEY_SENSOR_OPT_ID, opt_id, 1);

    std::vector<std::string> args{
      "INIT@SENSOR", std::to_string(static_cast<int>(sensor_id)), "1", std::to_string(static_cast<int>(opt_id))};
    configureSensor(args, *transport, encoder, true);
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
        encoder.write(*it, (*it)->currentPresetIdx());
    }
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback
