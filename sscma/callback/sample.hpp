#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

void run_sample(const std::string& cmd, int n_times, std::atomic<bool>& stop_token) {
    const auto& sensor_info  = static_resourse->device->get_sensor_info(static_resourse->current_sensor_id);
    auto        ret          = (sensor_info.id && sensor_info.state == EL_SENSOR_STA_AVAIL) ? EL_OK : EL_EINVAL;
    auto        direct_reply = [&]() {
        std::string ss(REPLY_CMD_HEADER);
        ss += concat_strings("\"name\": \"",
                             cmd,
                             "\", \"code\": ",
                             std::to_string(ret),
                             ", \"data\": {\"sensor\": ",
                             sensor_info_2_json_str(sensor_info),
                             "}}\n");
        static_resourse->transport->send_bytes(ss.c_str(), ss.size());
    };
    auto event_reply = [&](const std::string& sample_data_str) {
        std::string ss(REPLY_EVT_HEADER);
        ss += concat_strings(
          "\"name\": \"", cmd, "\", \"code\": ", std::to_string(ret), ", \"data\": {", sample_data_str, "}}\n");

        static_resourse->transport->send_bytes(ss.c_str(), ss.size());
    };

    if (ret != EL_OK) [[unlikely]]
        goto SampleErrorReply;

    if (sensor_info.type == EL_SENSOR_TYPE_CAM) {
        direct_reply();

        auto* camera = static_resourse->device->get_camera();
        auto  img    = el_img_t{.data   = nullptr,
                                .size   = 0,
                                .width  = 0,
                                .height = 0,
                                .format = EL_PIXEL_FORMAT_UNKNOWN,
                                .rotate = EL_PIXEL_ROTATE_UNKNOWN};

        static_resourse->is_sample.store(true);
        while ((n_times < 0 || --n_times >= 0) && !stop_token.load()) {
            ret = camera->start_stream();
            if (ret != EL_OK) [[unlikely]]
                goto SampleLoopErrorReply;

            ret = camera->get_frame(&img);
            if (ret != EL_OK) [[unlikely]]
                goto SampleLoopErrorReply;

            event_reply(img_2_jpeg_json_str(&img));

            camera->stop_stream();  // Note: discarding return err_code (always EL_OK)
            continue;

        SampleLoopErrorReply:
            event_reply("");
            break;
        }
        static_resourse->is_sample.store(false);

        return;
    } else
        ret = EL_EINVAL;

SampleErrorReply:
    direct_reply();
}

}  // namespace sscma::callback
