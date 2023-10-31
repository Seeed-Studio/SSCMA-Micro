#pragma once

#include <atomic>
#include <cstdint>
#include <forward_list>
#include <memory>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/traits.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::traits;
using namespace sscma::utility;

class Invoke final : public std::enable_shared_from_this<Invoke> {
   public:
    std::shared_ptr<Invoke> getptr() { return shared_from_this(); }

    [[nodiscard]] static std::shared_ptr<Invoke> create(std::string cmd, std::size_t n_times, bool results_only) {
        return std::shared_ptr<Invoke>{
          new Invoke{std::move(cmd), n_times, results_only}
        };
    }

    ~Invoke() { static_resource->is_invoke.store(false, std::memory_order_release); }

    inline void run() { prepare(); }

   protected:
    Invoke(std::string cmd, std::size_t n_times, bool results_only)
        : _cmd{cmd},
          _n_times{n_times},
          _results_only{results_only},
          _task_id{static_resource->current_task_id.load(std::memory_order_seq_cst)},
          _sensor_info{static_resource->device->get_sensor_info(static_resource->current_sensor_id)},
          _model_info{static_resource->models->get_model_info(static_resource->current_model_id)},
          _times{0},
          _ret{EL_OK},
          _action_hash{0} {
        static_resource->is_invoke.store(true, std::memory_order_release);
    }

   private:
    void prepare() {
        reset_action_hash();
        reset_config_cmds();
        check_sensor_status();
        check_model_status();
        prepare_algorithm_info();
        check_algorithm_info();
        event_loop();
    }

    inline void reset_action_hash() { _action_hash = 0; }

    inline void reset_config_cmds() {
        for (const auto& cmd : _config_cmds) static_resource->instance->unregister_cmd(cmd);
    }

    void prepare_algorithm_info() {
        if (static_resource->current_algorithm_type != EL_ALGO_TYPE_UNDEFINED)
            _algorithm_info =
              static_resource->algorithm_delegate->get_algorithm_info(static_resource->current_algorithm_type);
        else
            _algorithm_info = _model_info.type != EL_ALGO_TYPE_UNDEFINED
                                ? static_resource->algorithm_delegate->get_algorithm_info(_model_info.type)
                                : static_resource->algorithm_delegate->get_algorithm_info(
                                    el_algorithm_type_from_engine(static_resource->engine));
    }

    void check_sensor_status() {
        _ret = _sensor_info.id != 0 ? EL_OK : EL_EIO;
        if (_ret != EL_OK) [[unlikely]]
            return;
        _ret = _sensor_info.state == EL_SENSOR_STA_AVAIL ? EL_OK : EL_EIO;
    }

    void check_model_status() {
        if (_ret != EL_OK) [[unlikely]]
            return;
        _ret = _model_info.id != 0 ? EL_OK : EL_EINVAL;
    }

    void check_algorithm_info() {
        if (_ret != EL_OK) [[unlikely]]
            return;
        _ret = _algorithm_info.type != EL_ALGO_TYPE_UNDEFINED ? EL_OK : EL_EINVAL;
        if (_ret != EL_OK) [[unlikely]]
            return;
        _ret = _algorithm_info.input_from == _sensor_info.type ? EL_OK : EL_EINVAL;
    }

    inline void direct_reply(std::string algorithm_config) {
        const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                      _cmd,
                                      "\", \"code\": ",
                                      std::to_string(_ret),
                                      ", \"data\": {\"model\": ",
                                      model_info_2_json_str(_model_info),
                                      ", \"algorithm\": ",
                                      algorithm_config,
                                      ", \"sensor\": ",
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
        switch (_algorithm_info.type) {
        case EL_ALGO_TYPE_YOLO: {
            auto algorithm{std::make_shared<AlgorithmYOLO>(static_resource->engine)};
            register_config_cmds(algorithm);
            direct_reply(algorithm_config_2_json_str(algorithm));
            return event_loop_cam(algorithm);
        }
        default:
            _ret = EL_ENOTSUP;
            direct_reply(algorithm_info_2_json_str(&_algorithm_info));
        }
    }

    template <typename AlgorithmType> void register_config_cmds(std::shared_ptr<AlgorithmType> algorithm) {
        auto config = algorithm->get_algorithm_config();
        auto kv     = el_make_storage_kv_from_type(config);
        if (static_resource->storage->contains(kv.key)) [[likely]]
            *static_resource->storage >> kv;
        else
            *static_resource->storage << kv;
        algorithm->set_algorithm_config(kv.value);

        if constexpr (has_method_set_score_threshold<AlgorithmType>())
            if (static_resource->instance->register_cmd(
                  "TSCORE", "Set score threshold", "SCORE_THRESHOLD", [algorithm](std::vector<std::string> argv) {
                      uint8_t value     = std::atoi(argv[1].c_str());  // implicit conversion eliminates negtive values
                      el_err_code_t ret = value <= 100u ? EL_OK : EL_EINVAL;
                      static_resource->executor->add_task(
                        [algorithm, cmd = std::move(argv[0]), value, ret](const std::atomic<bool>&) mutable {
                            if (ret == EL_OK) [[likely]] {
                                algorithm->set_score_threshold(value);
                                ret = static_resource->storage->emplace(
                                        el_make_storage_kv_from_type(algorithm->get_algorithm_config()))
                                        ? EL_OK
                                        : EL_EIO;
                            }
                            const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                                          cmd,
                                                          "\", \"code\": ",
                                                          std::to_string(ret),
                                                          ", \"data\": ",
                                                          std::to_string(algorithm->get_score_threshold()),
                                                          "}\n")};
                            static_resource->transport->send_bytes(ss.c_str(), ss.size());
                        });
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TSCORE");

        if constexpr (has_method_get_score_threshold<AlgorithmType>())
            if (static_resource->instance->register_cmd(
                  "TSCORE?", "Get score threshold", "", [algorithm](std::vector<std::string> argv) {
                      static_resource->executor->add_task(
                        [algorithm, cmd = std::move(argv[0])](const std::atomic<bool>&) {
                            const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                                          cmd,
                                                          "\", \"code\": ",
                                                          std::to_string(EL_OK),
                                                          ", \"data\": ",
                                                          std::to_string(algorithm->get_score_threshold()),
                                                          "}\n")};
                            static_resource->transport->send_bytes(ss.c_str(), ss.size());
                        });
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TSCORE?");

        if constexpr (has_method_set_iou_threshold<AlgorithmType>())
            if (static_resource->instance->register_cmd(
                  "TIOU", "Set IoU threshold", "IOU_THRESHOLD", [algorithm](std::vector<std::string> argv) {
                      uint8_t value     = std::atoi(argv[1].c_str());  // implicit conversion eliminates negtive values
                      el_err_code_t ret = value <= 100u ? EL_OK : EL_EINVAL;
                      static_resource->executor->add_task(
                        [algorithm, cmd = std::move(argv[0]), value, ret](const std::atomic<bool>&) mutable {
                            if (ret == EL_OK) [[likely]] {
                                algorithm->set_iou_threshold(value);
                                ret = static_resource->storage->emplace(
                                        el_make_storage_kv_from_type(algorithm->get_algorithm_config()))
                                        ? EL_OK
                                        : EL_EIO;
                            }
                            const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                                          cmd,
                                                          "\", \"code\": ",
                                                          std::to_string(ret),
                                                          ", \"data\": ",
                                                          std::to_string(algorithm->get_iou_threshold()),
                                                          "}\n")};
                            static_resource->transport->send_bytes(ss.c_str(), ss.size());
                        });
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TIOU");

        if constexpr (has_method_get_iou_threshold<AlgorithmType>())
            if (static_resource->instance->register_cmd(
                  "TIOU?", "Get IoU threshold", "", [algorithm](std::vector<std::string> argv) {
                      static_resource->executor->add_task(
                        [algorithm, cmd = std::move(argv[0])](const std::atomic<bool>&) {
                            const auto& ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                                          cmd,
                                                          "\", \"code\": ",
                                                          std::to_string(EL_OK),
                                                          ", \"data\": ",
                                                          std::to_string(algorithm->get_iou_threshold()),
                                                          "}\n")};
                            static_resource->transport->send_bytes(ss.c_str(), ss.size());
                        });
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TIOU?");
    }

    template <typename AlgorithmType> void event_loop_cam(std::shared_ptr<AlgorithmType> algorithm) {
        if (_times++ == _n_times) [[unlikely]] {
            reset_config_cmds();
            return;
        }
        if (static_resource->current_task_id.load(std::memory_order_seq_cst) != _task_id) [[unlikely]] {
            reset_config_cmds();
            return;
        }
        if (static_resource->current_sensor_id != _sensor_info.id) [[unlikely]] {
            prepare();
            return;
        }
        if (static_resource->current_algorithm_type != EL_ALGO_TYPE_UNDEFINED &&
            static_resource->current_algorithm_type != _algorithm_info.type) [[unlikely]] {
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

        _ret = algorithm->run(&frame);
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        if (_action_hash != static_resource->action_cond->get_condition_hash()) [[unlikely]] {
            _action_hash = static_resource->action_cond->get_condition_hash();
            action_injection(algorithm);
        }
        static_resource->action_cond->evalute();

        if (_results_only)
            event_reply(
              concat_strings(", ", algorithm_results_2_json_str(algorithm), ", ", img_2_jpeg_json_str(&frame)));
        else
            event_reply(concat_strings(", ", img_2_jpeg_json_str(&frame)));

        _ret = camera->stop_stream();
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        static_resource->executor->add_task(
          [_this = std::move(getptr()), _algorithm = std::move(algorithm)](const std::atomic<bool>& stop_token) {
              if (stop_token.load(std::memory_order_seq_cst)) [[unlikely]]
                  return;
              _this->event_loop_cam(_algorithm);
          });
        return;

    Err:
        event_reply("");
    }

    template <typename AlgorithmType> void action_injection(std::shared_ptr<AlgorithmType> algorithm) {
        auto mutable_map = static_resource->action_cond->get_mutable_map();
        for (auto& kv : mutable_map) {
            const auto& argv = tokenize_function_2_argv(kv.first);
            if (!argv.size()) [[unlikely]]
                continue;

            if (argv[0] == "count") {
                // count items by default
                if (argv.size() == 1)
                    kv.second = [_algorithm = algorithm.get()]() -> int {
                        const auto& res = _algorithm->get_results();
                        return std::distance(res.begin(), res.end());
                    };
                // count items filtered by target id
                if (argv.size() == 3 && argv[1] == "target") {
                    uint8_t target = std::atoi(argv[2].c_str());
                    kv.second      = [_algorithm = algorithm.get(), target]() -> int {
                        size_t      init = 0;
                        const auto& res  = _algorithm->get_results();
                        for (const auto& v : res)
                            if (v.target == target) ++init;
                        return init;
                    };
                }
                continue;
            }

            if (argv[0] == "max_score") {
                // max score
                if (argv.size() == 1)
                    kv.second = [_algorithm = algorithm.get()]() -> int {
                        uint8_t     init = 0;
                        const auto& res  = _algorithm->get_results();
                        for (const auto& v : res)
                            if (v.score > init) init = v.score;
                        return init;
                    };
                // max score filtered by target id
                if (argv.size() == 3 && argv[1] == "target") {
                    uint8_t target = std::atoi(argv[2].c_str());
                    kv.second      = [_algorithm = algorithm.get(), target]() -> int {
                        uint8_t     init = 0;
                        const auto& res  = _algorithm->get_results();
                        for (const auto& v : res)
                            if (v.target == target && v.score > init) init = v.score;
                        return init;
                    };
                }
                continue;
            }
        }
        static_resource->action_cond->set_mutable_map(mutable_map);
    }

    inline bool is_everything_ok() const { return _ret == EL_OK; }

   private:
    std::string _cmd;
    std::size_t _n_times;
    bool        _results_only;

    std::size_t         _task_id;
    el_sensor_info_t    _sensor_info;
    el_model_info_t     _model_info;
    el_algorithm_info_t _algorithm_info;

    std::size_t   _times;
    el_err_code_t _ret;
    uint16_t      _action_hash;

    std::forward_list<std::string> _config_cmds;
};

}  // namespace sscma::callback
