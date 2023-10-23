#pragma once

#include <cstdint>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void get_available_sensors(const std::string& cmd) {
    const auto& registered_sensors = static_resourse->device->get_all_sensor_info();

    std::string ss{
      concat_strings(REPLY_CMD_HEADER, "\"name\": \"", cmd, "\", \"code\": ", std::to_string(EL_OK), ", \"data\": [")};
    DELIM_RESET;
    for (const auto& i : registered_sensors) {
        DELIM_PRINT(ss);
        ss += sensor_info_2_json_str(i);
    }
    ss += "]}\n";

    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

void set_sensor(const std::string& cmd, uint8_t sensor_id, bool enable) {
    auto sensor_info = static_resourse->device->get_sensor_info(sensor_id);

    auto ret = sensor_info.id ? EL_OK : EL_EINVAL;
    if (ret != EL_OK) [[unlikely]]
        goto SensorReply;

    // camera
    if (sensor_info.type == EL_SENSOR_TYPE_CAM) {
        auto* camera = static_resourse->device->get_camera();

        static_resourse->device->set_sensor_state(sensor_id, EL_SENSOR_STA_LOCKED);
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
        static_resourse->device->set_sensor_state(sensor_id, EL_SENSOR_STA_AVAIL);
        sensor_info = static_resourse->device->get_sensor_info(sensor_id);

        if (static_resourse->current_sensor_id != sensor_id) {
            static_resourse->current_sensor_id = sensor_id;
            if (static_resourse->is_ready.load()) [[likely]]
                *static_resourse->storage
                  << el_make_storage_kv("current_sensor_id", static_resourse->current_sensor_id);
        }
    } else
        ret = EL_ENOTSUP;
    goto SensorReply;

SensorError:
    static_resourse->current_sensor_id = 0;

SensorReply: {
    const auto& ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(ret),
                                  ", \"data\": {\"sensor\": ",
                                  sensor_info_2_json_str(sensor_info),
                                  "}}\n")};
    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}
}

void get_sensor_info(const std::string& cmd) {
    const auto& sensor_info = static_resourse->device->get_sensor_info(static_resourse->current_sensor_id);

    const auto& ss{concat_strings(REPLY_CMD_HEADER,
                                  "\"name\": \"",
                                  cmd,
                                  "\", \"code\": ",
                                  std::to_string(EL_OK),
                                  ", \"data\": ",
                                  sensor_info_2_json_str(sensor_info),
                                  "}\n")};
    static_resourse->transport->send_bytes(ss.c_str(), ss.size());
}

}  // namespace sscma::callback
