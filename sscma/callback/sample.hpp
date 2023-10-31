#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

class Sample final : public std::enable_shared_from_this<Sample> {
   public:
    std::shared_ptr<Sample> getptr() { return shared_from_this(); }

    [[nodiscard]] static std::shared_ptr<Sample> create(std::string cmd, std::size_t n_times) {
        return std::shared_ptr<Sample>{
          new Sample{std::move(cmd), n_times}
        };
    }

    ~Sample() { static_resource->is_sample.store(false, std::memory_order_release); }

    inline void run() { prepare(); }

   protected:
    Sample(std::string cmd, std::size_t n_times)
        : _cmd{cmd},
          _n_times{n_times},
          _task_id{static_resource->current_task_id.load(std::memory_order_seq_cst)},
          _sensor_info{static_resource->device->get_sensor_info(static_resource->current_sensor_id)},
          _times{0},
          _ret{EL_OK} {
        static_resource->is_sample.store(true, std::memory_order_release);
    }

   private:
    void prepare() {
        check_sensor_status();
        direct_reply();

        event_loop();
    }

    void check_sensor_status() {
        _ret = _sensor_info.id != 0 ? EL_OK : EL_EIO;
        if (_ret != EL_OK) [[unlikely]]
            return;
        _ret = _sensor_info.state == EL_SENSOR_STA_AVAIL ? EL_OK : EL_EIO;
    }

    inline void direct_reply() {
        const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                      _cmd,
                                      "\", \"code\": ",
                                      std::to_string(_ret),
                                      ", \"data\": {\"sensor\": ",
                                      sensor_info_2_json_str(_sensor_info),
                                      "}}\n")};
        static_resource->transport->send_bytes(ss.c_str(), ss.size());
    }

    inline void event_reply(std::string data) {
        const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"",
                                      _cmd,
                                      "\", \"code\": ",
                                      std::to_string(_ret),
                                      ", \"data\": {\"count\": ",
                                      std::to_string(_times),
                                      data,
                                      "}}\n")};
        static_resource->transport->send_bytes(ss.c_str(), ss.size());
    }

    void event_loop() {
        switch (_sensor_info.type) {
        case EL_SENSOR_TYPE_CAM:
            direct_reply();
            return event_loop_cam();
        default:
            _ret = EL_ENOTSUP;
            direct_reply();
        }
    }

    void event_loop_cam() {
        if (_times++ == _n_times) [[unlikely]]
            return;
        if (static_resource->current_task_id.load(std::memory_order_seq_cst) != _task_id) [[unlikely]]
            return;
        if (static_resource->current_sensor_id != _sensor_info.id) [[unlikely]] {
            prepare();
            return;
        }

        auto camera = static_resource->device->get_camera();
        auto frame  = el_img_t{};

        _ret = camera->start_stream();
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        _ret = camera->get_frame(&frame);
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        event_reply(concat_strings(", ", img_2_jpeg_json_str(&frame)));

        _ret = camera->stop_stream();
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        static_resource->executor->add_task(
          [_this = std::move(getptr())](const std::atomic<bool>&) { _this->event_loop_cam(); });
        return;

    Err:
        event_reply("");
    }

    inline bool is_everything_ok() const { return _ret == EL_OK; }

   private:
    std::string     _cmd;
    std::size_t     _n_times;

    std::size_t      _task_id;
    el_sensor_info_t _sensor_info;
    std::size_t      _times;

    el_err_code_t _ret;
};

}  // namespace sscma::callback
