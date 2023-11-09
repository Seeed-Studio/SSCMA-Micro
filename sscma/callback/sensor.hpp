#pragma once

#include <cstdint>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void get_available_sensors(const std::string& cmd) {
    const auto& registered_sensors = static_resource->device->get_all_sensor_info();
    const char* delim              = "";

    std::string ss{
      concat_strings("\r{\"type\": 0, \"name\": \"", cmd, "\", \"code\": ", std::to_string(EL_OK), ", \"data\": [")};

    for (const auto& i : registered_sensors) {
        ss += concat_strings(delim, sensor_info_2_json_str(i));
        delim = ", ";
    }
    ss += "]}\n";

    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void set_sensor(const std::string& cmd, uint8_t sensor_id, bool enable) {
    auto sensor_info = static_resource->device->get_sensor_info(sensor_id);

    auto ret = sensor_info.id ? EL_OK : EL_EINVAL;
    if (ret != EL_OK) [[unlikely]]
        goto SensorReply;

    // camera
    if (sensor_info.type == EL_SENSOR_TYPE_CAM) {
        auto* camera = static_resource->device->get_camera();

        static_resource->device->set_sensor_state(sensor_id, EL_SENSOR_STA_LOCKED);
        if (static_cast<bool>(*camera)) {
            ret = camera->deinit();
            if (ret != EL_OK) [[unlikely]]
                goto SensorError;
        }

        if (enable) [[likely]] {
            ret = camera->init(240, 240);  // TODO: custom resolution
            if (ret != EL_OK) [[unlikely]]
                goto SensorError;
        }
        static_resource->device->set_sensor_state(sensor_id, EL_SENSOR_STA_AVAIL);
        sensor_info = static_resource->device->get_sensor_info(sensor_id);

        if (static_resource->current_sensor_id != sensor_id) {
            static_resource->current_sensor_id = sensor_id;
            if (static_resource->is_ready.load()) [[likely]]
                *static_resource->storage
                  << el_make_storage_kv("current_sensor_id", static_resource->current_sensor_id);
        }
    } else
        ret = EL_ENOTSUP;
    goto SensorReply;

SensorError:
    static_resource->current_sensor_id = 0;

SensorReply:
#if CONFIG_EL_DEBUG == 0
    if (!static_resource->is_ready.load()) return;
#endif

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"sensor\": ",
                                  sensor_info_2_json_str(sensor_info),
                                  "}}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

void get_sensor_info(const std::string& cmd) {
    const auto& sensor_info = static_resource->device->get_sensor_info(static_resource->current_sensor_id);

    const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": ",
                                  sensor_info_2_json_str(sensor_info),
                                  "}\n")};
    static_resource->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
