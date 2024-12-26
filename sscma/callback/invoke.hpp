#pragma once

#include <atomic>
#include <cstdint>
#include <forward_list>
#include <memory>
#include <string>

#include "core/algorithm/el_algorithm_delegate.h"
#include "extension/contents_export.hpp"
#include "extension/results_filter.hpp"
#include "sscma/definations.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/traits.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::extension;
using namespace sscma::traits;
using namespace sscma::utility;

class Invoke final : public std::enable_shared_from_this<Invoke> {
   public:
    std::shared_ptr<Invoke> getptr() { return shared_from_this(); }

    [[nodiscard]] static std::shared_ptr<Invoke> create(
      std::string cmd, int32_t n_times, bool differed, bool results_only, void* caller) {
        return std::shared_ptr<Invoke>{
          new Invoke{std::move(cmd), n_times, differed, results_only, caller}
        };
    }

    ~Invoke() {
        reset_config_cmds();
        static_resource->is_invoke = false;
    }

    inline void run() { prepare(); }

   protected:
    Invoke(std::string cmd, int32_t n_times, bool differed, bool results_only, void* caller)
        : _cmd{cmd},
          _n_times{n_times},
          _differed{differed},
          _results_only{results_only},
          _caller{caller},
          _task_id{static_resource->current_task_id.load(std::memory_order_seq_cst)},
          _sensor_info{},
          _model_info{},
          _algorithm_info{},
          _times{0},
          _ret{EL_OK}
#if SSCMA_CFG_ENABLE_ACTION
          ,
          _action_hash{0}
#endif
#if SSCMA_CFG_ENABLE_CONTENTS_EXPORT
          ,
          _contents_export{false}
#endif
    {
        static_resource->is_invoke = true;
#if SSCMA_CFG_ENABLE_CONTENTS_EXPORT
        static auto exporter{std::make_shared<ContentsExport>()};
        _exporter = exporter;
#endif
    }

   private:
    inline void prepare() {
        reset_action_hash();
        reset_config_cmds();
        prepare_sensor_info();
        if (!check_sensor_status()) [[unlikely]]
            goto Err;
        prepare_model_info();
        if (!check_model_status()) [[unlikely]]
            goto Err;
        prepare_algorithm_info();
        if (!check_algorithm_info()) [[unlikely]]
            goto Err;

        static_resource->executor->add_task(
          [_this = std::move(getptr())](const std::atomic<bool>&) { _this->event_loop(); });
        return;

    Err:
        direct_reply(algorithm_info_2_json_str(&_algorithm_info));
    }

    inline void reset_action_hash() {
#if SSCMA_CFG_ENABLE_ACTION
        _action_hash = 0;
#endif
    }

    inline void reset_config_cmds() {
        for (const auto& cmd : _config_cmds) static_resource->instance->unregister_cmd(cmd);
    }

    inline void prepare_sensor_info() {
        _sensor_info = static_resource->device->get_sensor_info(static_resource->current_sensor_id);
    }

    inline void prepare_model_info() {
        _model_info = static_resource->models->get_model_info(static_resource->current_model_id);
    }

    inline void prepare_algorithm_info() {
        if (static_resource->current_algorithm_type != EL_ALGO_TYPE_UNDEFINED)
            _algorithm_info =
              static_resource->algorithm_delegate->get_algorithm_info(static_resource->current_algorithm_type);
        else
            _algorithm_info = _model_info.type != EL_ALGO_TYPE_UNDEFINED
                                ? static_resource->algorithm_delegate->get_algorithm_info(_model_info.type)
                                : static_resource->algorithm_delegate->get_algorithm_info(
                                    el_algorithm_type_from_engine(static_resource->engine));
    }

    inline bool check_sensor_status() {
        _ret = _sensor_info.id != 0 ? EL_OK : EL_EIO;
        if (_ret != EL_OK) [[unlikely]]
            return false;
        _ret = _sensor_info.state == EL_SENSOR_STA_AVAIL ? EL_OK : EL_EIO;
        if (_ret != EL_OK) [[unlikely]]
            return false;
        return true;
    }

    inline bool check_model_status() {
        _ret = _model_info.id != 0 ? EL_OK : EL_EINVAL;
        if (_ret != EL_OK) [[unlikely]]
            return false;
        _ret = static_resource->engine ? EL_OK : EL_EPERM;
        if (_ret != EL_OK) [[unlikely]]
            return false;
        return true;
    }

    inline bool check_algorithm_info() {
        _ret = _algorithm_info.type != EL_ALGO_TYPE_UNDEFINED ? EL_OK : EL_EINVAL;
        if (_ret != EL_OK) [[unlikely]]
            return false;
        _ret = _algorithm_info.input_from == _sensor_info.type ? EL_OK : EL_EINVAL;
        if (_ret != EL_OK) [[unlikely]]
            return false;
        return true;
    }

    inline void direct_reply(std::string algorithm_config) {
        auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                               _cmd,
                               "\", \"code\": ",
                               std::to_string(_ret),
                               ", \"data\": {\"model\": ",
                               model_info_2_json_str(_model_info),
                               ", \"algorithm\": ",
                               algorithm_config,
                               ", \"sensor\": ",
                               sensor_info_2_json_str(_sensor_info, static_resource->device),
                               "}}\n")};
        static_cast<Transport*>(_caller)->send_bytes(ss.c_str(), ss.size());
    }

    inline void event_reply(std::string data) {
        {
            auto ss{concat_strings("\r{\"type\": 1, \"name\": \"",
                                   _cmd,
                                   "\", \"code\": ",
                                   std::to_string(_ret),
                                   ", \"data\": {\"count\": ",
                                   std::to_string(_times))};
            static_cast<Transport*>(_caller)->send_bytes(ss.c_str(), ss.size());
        }
        {
            auto hdr_pos = data.find("\"image\": \"\"");
            if (hdr_pos != std::string::npos) {
                static_cast<Transport*>(_caller)->send_bytes(data.c_str(), hdr_pos + 10);
                static_cast<Transport*>(_caller)->send_bytes(_internal_jpeg_buffer, _internal_jpeg_buffer_size);
                auto hdr = data.substr(hdr_pos + 10);
                static_cast<Transport*>(_caller)->send_bytes(hdr.c_str(), hdr.size());
            } else {
                static_cast<Transport*>(_caller)->send_bytes(data.c_str(), data.size());
            }
        }
        static_cast<Transport*>(_caller)->send_bytes("}}\n", 3);
    }

    inline void event_loop() {
        switch (_algorithm_info.type) {
        case EL_ALGO_TYPE_FOMO: {
            using AlgorithmType = AlgorithmFOMO;
            auto algorithm{std::make_shared<AlgorithmType>(static_resource->engine)};
            register_config_cmds(algorithm);
            direct_reply(algorithm_config_2_json_str(algorithm));
            if (is_everything_ok()) [[likely]] {
                auto results_filter{ResultsFilter(algorithm->get_results())};
                event_loop_cam(algorithm, std::move(results_filter));
            }
            return;
        }
// Depraecate PFLD to avoid firmware generation error
#ifndef CONFIG_EL_TARGET_HIMAX
        case EL_ALGO_TYPE_PFLD: {
            using AlgorithmType = AlgorithmPFLD;
            auto algorithm{std::make_shared<AlgorithmType>(static_resource->engine)};
            register_config_cmds(algorithm);
            direct_reply(algorithm_config_2_json_str(algorithm));
            if (is_everything_ok()) [[likely]] {
                auto results_filter{ResultsFilter(algorithm->get_results())};
                event_loop_cam(algorithm, std::move(results_filter));
            }
            return;
        }
#endif
        case EL_ALGO_TYPE_YOLO: {
            using AlgorithmType = AlgorithmYOLO;
            auto algorithm{std::make_shared<AlgorithmType>(static_resource->engine)};
            register_config_cmds(algorithm);
            direct_reply(algorithm_config_2_json_str(algorithm));
            if (is_everything_ok()) [[likely]] {
                auto results_filter{ResultsFilter(algorithm->get_results())};
                event_loop_cam(algorithm, std::move(results_filter));
            }
            return;
        }
        case EL_ALGO_TYPE_IMCLS: {
            using AlgorithmType = AlgorithmIMCLS;
            auto algorithm{std::make_shared<AlgorithmType>(static_resource->engine)};
            register_config_cmds(algorithm);
            direct_reply(algorithm_config_2_json_str(algorithm));
            if (is_everything_ok()) [[likely]] {
                auto results_filter{ResultsFilter(algorithm->get_results())};
                event_loop_cam(algorithm, std::move(results_filter));
            }
            return;
        }
        case EL_ALGO_TYPE_YOLO_POSE: {
            using AlgorithmType = AlgorithmYOLOPOSE;
            auto algorithm{std::make_shared<AlgorithmType>(static_resource->engine)};
            register_config_cmds(algorithm);
            direct_reply(algorithm_config_2_json_str(algorithm));
            if (is_everything_ok()) [[likely]] {
                auto results_filter{ResultsFilter(algorithm->get_results())};
                event_loop_cam(algorithm, std::move(results_filter));
            }
            return;
        }
        case EL_ALGO_TYPE_YOLO_V8: {
            using AlgorithmType = AlgorithmYOLOV8;
            auto algorithm{std::make_shared<AlgorithmType>(static_resource->engine)};
            register_config_cmds(algorithm);
            direct_reply(algorithm_config_2_json_str(algorithm));
            if (is_everything_ok()) [[likely]] {
                auto results_filter{ResultsFilter(algorithm->get_results())};
                event_loop_cam(algorithm, std::move(results_filter));
            }
            return;
        }
        case EL_ALGO_TYPE_YOLO_WORLD: {
            using AlgorithmType = AlgorithmYOLOWorld;
            auto algorithm{std::make_shared<AlgorithmType>(static_resource->engine)};
            register_config_cmds(algorithm);
            direct_reply(algorithm_config_2_json_str(algorithm));
            if (is_everything_ok()) [[likely]] {
                auto results_filter{ResultsFilter(algorithm->get_results())};
                event_loop_cam(algorithm, std::move(results_filter));
            }
            return;
        }
        case EL_ALGO_TYPE_NVIDIA_DET: {
            using AlgorithmType = AlgorithmNvidiaDet;
            auto algorithm{std::make_shared<AlgorithmType>(static_resource->engine)};
            register_config_cmds(algorithm);
            direct_reply(algorithm_config_2_json_str(algorithm));
            if (is_everything_ok()) [[likely]] {
                auto results_filter{ResultsFilter(algorithm->get_results())};
                event_loop_cam(algorithm, std::move(results_filter));
            }
            return;
        }

        default:
            _ret = EL_ENOTSUP;
            direct_reply(algorithm_info_2_json_str(&_algorithm_info));
        }
    }

    template <typename AlgorithmType> constexpr void register_config_cmds(std::shared_ptr<AlgorithmType> algorithm) {
        auto config = algorithm->get_algorithm_config();
        auto kv     = el_make_storage_kv_from_type(config);
        if (static_resource->storage->contains(kv.key)) [[likely]]
            *static_resource->storage >> kv;
        else
            *static_resource->storage << kv;
        algorithm->set_algorithm_config(kv.value);

        if constexpr (has_method_set_score_threshold<AlgorithmType>())
            if (static_resource->instance->register_cmd(
                  "TSCORE",
                  "Set score threshold",
                  "SCORE_THRESHOLD",
                  [algorithm](std::vector<std::string> argv, void* caller) {
                      uint8_t value     = std::atoi(argv[1].c_str());  // implicit conversion eliminates negtive values
                      el_err_code_t ret = value <= 100u ? EL_OK : EL_EINVAL;
                      static_resource->executor->add_task(
                        [algorithm, cmd = std::move(argv[0]), value, ret, caller](const std::atomic<bool>&) mutable {
                            if (ret == EL_OK) [[likely]] {
                                if (algorithm->get_score_threshold() != value) [[likely]] {
                                    algorithm->set_score_threshold(value);
                                    ret = static_resource->storage->emplace(
                                            el_make_storage_kv_from_type(algorithm->get_algorithm_config()))
                                            ? EL_OK
                                            : EL_EIO;
                                }
                            }
                            auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                                   cmd,
                                                   "\", \"code\": ",
                                                   std::to_string(ret),
                                                   ", \"data\": ",
                                                   std::to_string(algorithm->get_score_threshold()),
                                                   "}\n")};
                            static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
                            // caller, buffer, cahr* id
                        });
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TSCORE");

        if constexpr (has_method_get_score_threshold<AlgorithmType>())
            if (static_resource->instance->register_cmd(
                  "TSCORE?", "Get score threshold", "", [algorithm](std::vector<std::string> argv, void* caller) {
                      static_resource->executor->add_task(
                        [algorithm, cmd = std::move(argv[0]), caller](const std::atomic<bool>&) {
                            auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                                   cmd,
                                                   "\", \"code\": ",
                                                   std::to_string(EL_OK),
                                                   ", \"data\": ",
                                                   std::to_string(algorithm->get_score_threshold()),
                                                   "}\n")};
                            static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
                        });
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TSCORE?");

        if constexpr (has_method_set_iou_threshold<AlgorithmType>())
            if (static_resource->instance->register_cmd(
                  "TIOU",
                  "Set IoU threshold",
                  "IOU_THRESHOLD",
                  [algorithm](std::vector<std::string> argv, void* caller) {
                      uint8_t value     = std::atoi(argv[1].c_str());  // implicit conversion eliminates negtive values
                      el_err_code_t ret = value <= 100u ? EL_OK : EL_EINVAL;
                      static_resource->executor->add_task(
                        [algorithm, cmd = std::move(argv[0]), value, ret, caller](const std::atomic<bool>&) mutable {
                            if (ret == EL_OK) [[likely]] {
                                if (algorithm->get_iou_threshold() != value) [[likely]] {
                                    algorithm->set_iou_threshold(value);
                                    ret = static_resource->storage->emplace(
                                            el_make_storage_kv_from_type(algorithm->get_algorithm_config()))
                                            ? EL_OK
                                            : EL_EIO;
                                }
                            }
                            auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                                   cmd,
                                                   "\", \"code\": ",
                                                   std::to_string(ret),
                                                   ", \"data\": ",
                                                   std::to_string(algorithm->get_iou_threshold()),
                                                   "}\n")};
                            static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
                        });
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TIOU");

        if constexpr (has_method_get_iou_threshold<AlgorithmType>())
            if (static_resource->instance->register_cmd(
                  "TIOU?", "Get IoU threshold", "", [algorithm](std::vector<std::string> argv, void* caller) {
                      static_resource->executor->add_task(
                        [algorithm, cmd = std::move(argv[0]), caller](const std::atomic<bool>&) {
                            auto ss{concat_strings("\r{\"type\": 0, \"name\": \"",
                                                   cmd,
                                                   "\", \"code\": ",
                                                   std::to_string(EL_OK),
                                                   ", \"data\": ",
                                                   std::to_string(algorithm->get_iou_threshold()),
                                                   "}\n")};
                            static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
                        });
                      return EL_OK;
                  }) == EL_OK) [[likely]]
                _config_cmds.emplace_front("TIOU?");
    }

    template <typename AlgorithmType, typename ResultType = typename AlgorithmType::OutputType>
    void event_loop_cam(std::shared_ptr<AlgorithmType> algorithm, ResultsFilter<ResultType> results_filter) {
        if ((_n_times >= 0) & (_times++ >= _n_times)) [[unlikely]]
            return;
        if (static_resource->current_task_id.load(std::memory_order_seq_cst) != _task_id) [[unlikely]]
            return;

        auto camera            = static_resource->device->get_camera();
        auto frame             = el_img_t{};
        auto processed_frame   = el_img_t{};
        auto encoded_frame_str = std::string{};
#if SSCMA_CFG_ENABLE_ACTION
        if (_action_hash != static_resource->action->get_condition_hash()) [[unlikely]] {
            _action_hash = static_resource->action->get_condition_hash();
            action_injection(algorithm);
        }
#endif

        _ret = camera->start_stream();
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        _ret = camera->get_frame(&frame);
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        if (!_results_only
#if SSCMA_CFG_ENABLE_CONTENTS_EXPORT
            || _contents_export
#endif
        ) {
#if CONFIG_EL_HAS_ACCELERATED_JPEG_CODEC
            _ret = camera->get_processed_frame(&processed_frame);
            if (!is_everything_ok()) [[unlikely]]
                goto Err;
            encoded_frame_str = std::move(img_2_json_str(&processed_frame));
#else
            encoded_frame_str = std::move(img_2_jpeg_json_str(&frame, &processed_frame));
#endif
        }

        _ret = algorithm->run(&frame);
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

#if SSCMA_CFG_ENABLE_CONTENTS_EXPORT
        if (_contents_export && _exporter) [[likely]] {
            _exporter->cache(processed_frame.data, processed_frame.size);
            _exporter->cache_result(concat_strings("\r{\"type\": 1, \"name\": \"",
                                                   _cmd,
                                                   "\", \"code\": ",
                                                   std::to_string(_ret),
                                                   ", \"data\": {\"count\": ",
                                                   std::to_string(_times),
                                                   ", ",
                                                   algorithm_results_2_json_str(algorithm),
                                                   ", ",
                                                   img_res_2_json_str(&frame),
                                                   "}}\n"));
        }
#endif

#if SSCMA_CFG_ENABLE_ACTION
        static_resource->action->evalute(_caller);
#endif

        _ret = camera->stop_stream();
        if (!is_everything_ok()) [[unlikely]]
            goto Err;

        if (!_differed || results_filter.compare_and_update(algorithm->get_results())) {
            if (_results_only)
                event_reply(
                  concat_strings(", ", algorithm_results_2_json_str(algorithm), ", ", img_res_2_json_str(&frame)));
            else {
                event_reply(concat_strings(", ",
                                           algorithm_results_2_json_str(algorithm),
                                           ", ",
                                           img_res_2_json_str(&frame),
                                           ", ",
                                           std::move(encoded_frame_str)));
            }
        }

        static_resource->executor->add_task(
          [_this = std::move(getptr()), _algorithm = std::move(algorithm), _results_filter = std::move(results_filter)](
            const std::atomic<bool>& stop_token) {
              if (stop_token.load(std::memory_order_seq_cst)) [[unlikely]]
                  return;
              _this->event_loop_cam(_algorithm, std::move(_results_filter));
          });
        return;

    Err:
        event_reply("");
    }

#if SSCMA_CFG_ENABLE_ACTION
    template <typename AlgorithmType> void action_injection(std::shared_ptr<AlgorithmType> algorithm) {
    #if SSCMA_CFG_ENABLE_CONTENTS_EXPORT
        // reset action related flags
        _contents_export = false;
    #endif

        auto mutable_map = static_resource->action->get_mutable_map();
        for (auto& kv : mutable_map) {
            auto argv = tokenize_function_2_argv(kv.first);
            if (!argv.size()) [[unlikely]]
                continue;

            if (argv[0] == "count") {
                // count items by default
                if (argv.size() == 1)
                    kv.second = [_algorithm = algorithm.get()](void*) -> int {
                        const auto& res = _algorithm->get_results();
                        return std::distance(res.begin(), res.end());
                    };
                // count items filtered by target id
                if (argv.size() == 3 && argv[1] == "target") {
                    uint8_t target = std::atoi(argv[2].c_str());
                    kv.second      = [_algorithm = algorithm.get(), target](void*) -> int {
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
                    kv.second = [_algorithm = algorithm.get()](void*) -> int {
                        uint8_t     init = 0;
                        const auto& res  = _algorithm->get_results();
                        for (const auto& v : res)
                            if (v.score > init) init = v.score;
                        return init;
                    };
                // max score filtered by target id
                if (argv.size() == 3 && argv[1] == "target") {
                    uint8_t target = std::atoi(argv[2].c_str());
                    kv.second      = [_algorithm = algorithm.get(), target](void*) -> int {
                        uint8_t     init = 0;
                        const auto& res  = _algorithm->get_results();
                        for (const auto& v : res)
                            if (v.target == target && v.score > init) init = v.score;
                        return init;
                    };
                }
                continue;
            }

    #if SSCMA_CFG_ENABLE_CONTENTS_EXPORT
            if (argv[0] == "save_jpeg") {
                // save jpeg
                if (argv.size() == 1) {
                    _contents_export = true;
                    std::string cmd  = _cmd;
                    kv.second        = [exporter = _exporter, cmd = std::move(cmd)](void* caller) -> int {
                        bool        success   = false;
                        uint32_t    time      = el_get_time_ms();
                        std::string file_name = std::to_string(time);

                        if (!exporter) [[unlikely]]
                            return 0;

                        success = exporter->commit(file_name, [caller, cmd = std::move(cmd)](Status status) {
                            if (status.success) return;
                            auto ss{concat_strings("\r{\"type\": 1, \"name\": ",
                                                   quoted(cmd + "@ACTION"),
                                                   ", code\": ",
                                                   std::to_string(EL_EIO),
                                                   ", \"data\": ",
                                                   quoted(status.message),
                                                   "}\n")};
                            static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
                        });

                        return success ? 1 : 0;
                    };
                }
                continue;
            }
    #endif
        }
        static_resource->action->set_mutable_map(mutable_map);
    }
#endif

    inline bool is_everything_ok() const { return _ret == EL_OK; }

   private:
    std::string _cmd;
    int32_t     _n_times;
    bool        _differed;
    bool        _results_only;
    void*       _caller;

    std::size_t         _task_id;
    el_sensor_info_t    _sensor_info;
    el_model_info_t     _model_info;
    el_algorithm_info_t _algorithm_info;

    int32_t       _times;
    el_err_code_t _ret;

#if SSCMA_CFG_ENABLE_ACTION
    uint16_t _action_hash;
#endif

#if SSCMA_CFG_ENABLE_CONTENTS_EXPORT
    bool                            _contents_export;
    std::shared_ptr<ContentsExport> _exporter;
#endif

    std::forward_list<std::string> _config_cmds;
};

}  // namespace sscma::callback
