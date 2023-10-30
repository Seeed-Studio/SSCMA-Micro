#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

class Sample final : public std::enable_shared_from_this<Sample> {
   public:
    std::shared_ptr<Sample> getptr() { return shared_from_this(); }

    [[nodiscard]] static std::shared_ptr<Sample> create(StaticResourse* observable_resource,
                                                        std::string     cmd,
                                                        std::size_t     n_times) {
        return std::shared_ptr<Sample>{
          new Sample{observable_resource, std::move(cmd), n_times}
        };
    }

    ~Sample() { _observable_resource->is_sample.store(false, std::memory_order_release); };

    void run() { prepare(true); }

   protected:
    Sample(StaticResourse* observable_resource, std::string cmd, std::size_t n_times)
        : _observable_resource{observable_resource},
          _cmd{cmd},
          _n_times{n_times},
          _task_id{observable_resource->current_task_id.load(std::memory_order_seq_cst)},
          _sensor_info{observable_resource->device->get_sensor_info(observable_resource->current_sensor_id)},
          _times{0},
          _ret{EL_OK} {
        _observable_resource->is_sample.store(true, std::memory_order_release);
    }

   private:
    void prepare(bool has_reply) {
        check_sensor_status();
        if (has_reply) direct_reply();
        if (is_everything_ok())
            event_loop();
        else
            event_reply();
    }

    void check_sensor_status() {
        _ret = _sensor_info.id != 0 ? EL_OK : EL_EIO;
        if (_ret != EL_OK) [[unlikely]]
            return;
        _ret = _sensor_info.state == EL_SENSOR_STA_AVAIL ? EL_OK : EL_EIO;
        if (_ret != EL_OK) [[unlikely]]
            return;
    }

    inline void direct_reply() {
        const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                      _cmd,
                                      "\", \"code\": ",
                                      std::to_string(_ret),
                                      ", \"data\": {\"sensor\": ",
                                      sensor_info_2_json_str(_sensor_info),
                                      "}}\n")};
        _observable_resource->transport->send_bytes(ss.c_str(), ss.size());
    }

    inline void event_reply() {
        const auto& ss{concat_strings("\r{\"type\": 1, \"name\": \"",
                                      _cmd,
                                      "\", \"code\": ",
                                      std::to_string(_ret),
                                      ", \"data\": {\"count\": ",
                                      std::to_string(_times),
                                      ", ",
                                      _data,
                                      "}}\n")};
        _observable_resource->transport->send_bytes(ss.c_str(), ss.size());
    }

    void event_loop() {
        switch (_sensor_info.type) {
        case EL_SENSOR_TYPE_CAM:
            return event_loop_cam();
        default:
            _ret = EL_ENOTSUP;
            event_reply();
        }
    }

    void event_loop_cam() {
        if (_times++ == _n_times) [[unlikely]]
            return;
        if (_observable_resource->current_task_id.load(std::memory_order_seq_cst) != _task_id) [[unlikely]]
            return;
        if (_observable_resource->current_sensor_id != _sensor_info.id) [[unlikely]] {
            prepare(false);
            return;
        }

        auto camera = _observable_resource->device->get_camera();
        auto frame  = el_img_t{};

        _ret = camera->start_stream();
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        _ret = camera->get_frame(&frame);
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        _data = img_2_jpeg_json_str(&frame);
        event_reply();

        _ret = camera->stop_stream();
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        _observable_resource->executor->add_task([_this = std::move(getptr())](const std::atomic<bool>& stop_token) {
            if (stop_token.load(std::memory_order_seq_cst)) [[unlikely]]
                return;
            _this->event_loop_cam();
        });
        return;

    Err:
        event_reply();
    }

    inline bool is_everything_ok() const { return _ret == EL_OK; }

   private:
    StaticResourse*  _observable_resource;
    std::string      _cmd;
    std::size_t      _n_times;
    std::size_t      _task_id;
    el_sensor_info_t _sensor_info;
    std::size_t      _times;
    el_err_code_t    _ret;
    std::string      _data;
};

}  // namespace sscma::callback
