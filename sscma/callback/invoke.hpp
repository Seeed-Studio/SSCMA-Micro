#pragma once

#include <atomic>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include "sscma/definations.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/utility.hpp"

namespace sscma::callback {

using namespace sscma::utility;

template <typename AlgorithmType>
void run_invoke_on_img(
  AlgorithmType* algorithm, const std::string& cmd, int n_times, bool result_only, std::atomic<bool>& stop_token) {
    auto* camera      = static_resourse->device->get_camera();
    auto  img         = el_img_t{.data   = nullptr,
                                 .size   = 0,
                                 .width  = 0,
                                 .height = 0,
                                 .format = EL_PIXEL_FORMAT_UNKNOWN,
                                 .rotate = EL_PIXEL_ROTATE_UNKNOWN};
    auto  ret         = algorithm ? EL_OK : EL_EINVAL;
    auto  event_reply = [&]() {
        const auto& str = img_invoke_results_2_json_str(algorithm, &img, cmd, result_only, ret);
        static_resourse->transport->send_bytes(str.c_str(), str.size());
    };
    if (ret != EL_OK) [[unlikely]] {
        event_reply();
        return;
    }
    {
        auto mutable_map = static_resourse->action_cond->get_mutable_map();
        for (auto& kv : mutable_map) {
            const auto& argv = tokenize_function_2_argv(kv.first);

            if (!argv.size()) [[unlikely]]
                continue;

            if (argv[0] == "count") {
                // count items by default
                if (argv.size() == 1)
                    kv.second = [=]() -> int {
                        const auto& res = algorithm->get_results();
                        return std::distance(res.begin(), res.end());
                    };

                // count items filtered by target id
                if (argv.size() == 3 && argv[1] == "target") {
                    uint8_t target = std::atoi(argv[2].c_str());
                    kv.second      = [=]() -> int {
                        size_t      init = 0;
                        const auto& res  = algorithm->get_results();
                        for (const auto& v : res)
                            if (v.target == target) ++init;
                        return init;
                    };
                }
            } else if (argv[0] == "max_score") {
                // max score
                if (argv.size() == 1)
                    kv.second = [=]() -> int {
                        uint8_t     init = 0;
                        const auto& res  = algorithm->get_results();
                        for (const auto& v : res)
                            if (v.score > init) init = v.score;
                        return init;
                    };

                // max score filtered by target id
                if (argv.size() == 3 && argv[1] == "target") {
                    uint8_t target = std::atoi(argv[2].c_str());
                    kv.second      = [=]() -> int {
                        uint8_t     init = 0;
                        const auto& res  = algorithm->get_results();
                        for (const auto& v : res)
                            if (v.target == target && v.score > init) init = v.score;

                        return init;
                    };
                }
            }
        }
        static_resourse->action_cond->set_mutable_map(mutable_map);
    }

    static_resourse->is_invoke.store(true);
    while ((n_times < 0 || --n_times >= 0) && !stop_token.load()) {
        ret = camera->start_stream();
        if (ret != EL_OK) [[unlikely]]
            goto InvokeErrorReply;

        ret = camera->get_frame(&img);
        if (ret != EL_OK) [[unlikely]]
            goto InvokeErrorReply;

        ret = algorithm->run(&img);
        if (ret != EL_OK) [[unlikely]]
            goto InvokeErrorReply;

        static_resourse->action_cond->evalute();

        event_reply();
        camera->stop_stream();  // Note: discarding return err_code (always EL_OK)
        continue;

    InvokeErrorReply:
        camera->stop_stream();
        event_reply();
        break;
    }
    static_resourse->is_invoke.store(false);
}

void run_invoke(const std::string& cmd, int n_times, bool result_only, std::atomic<bool>& stop_token) {
    auto os             = std::ostringstream(std::ios_base::ate);
    auto model_info     = el_model_info_t{};
    auto algorithm_info = el_algorithm_info_t{};
    auto sensor_info    = el_sensor_info_t{};
    auto ret            = EL_OK;
    auto direct_reply   = [&](const std::string& algorithm_info_and_conf) {
        os << REPLY_CMD_HEADER << "\"name\": \"" << cmd << "\", \"code\": " << static_cast<int>(ret)
           << ", \"data\": {\"model\": " << model_info_2_json_str(model_info)
           << ", \"algorithm\": " << algorithm_info_and_conf << ", \"sensor\": " << sensor_info_2_json_str(sensor_info)
           << "}}\n";

        const auto& str{os.str()};
        static_resourse->transport->send_bytes(str.c_str(), str.size());
    };

    model_info = static_resourse->models->get_model_info(static_resourse->current_model_id);
    if (model_info.id == 0) [[unlikely]]
        goto InvokeErrorReply;

    if (static_resourse->current_algorithm_type != EL_ALGO_TYPE_UNDEFINED)
        algorithm_info =
          static_resourse->algorithm_delegate->get_algorithm_info(static_resourse->current_algorithm_type);
    else
        algorithm_info = model_info.type != EL_ALGO_TYPE_UNDEFINED
                           ? static_resourse->algorithm_delegate->get_algorithm_info(model_info.type)
                           : static_resourse->algorithm_delegate->get_algorithm_info(
                               el_algorithm_type_from_engine(static_resourse->engine));
    if (algorithm_info.type == EL_ALGO_TYPE_UNDEFINED) [[unlikely]]
        goto InvokeErrorReply;

    sensor_info =
      static_resourse->device->get_sensor_info(static_resourse->current_sensor_id, algorithm_info.input_from);
    if (sensor_info.id == 0 || sensor_info.state != EL_SENSOR_STA_AVAIL) [[unlikely]]
        goto InvokeErrorReply;

    switch (algorithm_info.type) {
    case EL_ALGO_TYPE_IMCLS: {
        if (!AlgorithmIMCLS::is_model_valid(static_resourse->engine)) [[unlikely]]
            break;
        std::unique_ptr<AlgorithmIMCLS> algorithm(new AlgorithmIMCLS(static_resourse->engine));

        auto algo_config_helper{AlgorithmConfigHelper<AlgorithmIMCLS>(algorithm.get())};
        direct_reply(algorithm_info_and_conf_2_json_str(algo_config_helper.dump_config()));

        run_invoke_on_img(algorithm.get(), cmd, n_times, result_only, stop_token);
    }
        return;

    case EL_ALGO_TYPE_FOMO: {
        if (!AlgorithmFOMO::is_model_valid(static_resourse->engine)) [[unlikely]]
            break;
        std::unique_ptr<AlgorithmFOMO> algorithm(new AlgorithmFOMO(static_resourse->engine));

        auto algo_config_helper{AlgorithmConfigHelper<AlgorithmFOMO>(algorithm.get())};
        direct_reply(algorithm_info_and_conf_2_json_str(algo_config_helper.dump_config()));

        run_invoke_on_img(algorithm.get(), cmd, n_times, result_only, stop_token);
    }
        return;

    case EL_ALGO_TYPE_PFLD: {
        if (!AlgorithmPFLD::is_model_valid(static_resourse->engine)) [[unlikely]]
            break;
        std::unique_ptr<AlgorithmPFLD> algorithm(new AlgorithmPFLD(static_resourse->engine));
        direct_reply(algorithm_info_and_conf_2_json_str(algorithm->get_algorithm_config()));

        run_invoke_on_img(algorithm.get(), cmd, n_times, result_only, stop_token);
    }
        return;

    case EL_ALGO_TYPE_YOLO: {
        if (!AlgorithmYOLO::is_model_valid(static_resourse->engine)) [[unlikely]]
            break;
        std::unique_ptr<AlgorithmYOLO> algorithm(new AlgorithmYOLO(static_resourse->engine));

        auto algo_config_helper{AlgorithmConfigHelper<AlgorithmYOLO>(algorithm.get())};
        direct_reply(algorithm_info_and_conf_2_json_str(algo_config_helper.dump_config()));

        run_invoke_on_img(algorithm.get(), cmd, n_times, result_only, stop_token);
    }
        return;

    default:
        ret = EL_ENOTSUP;
    }

InvokeErrorReply:
    ret = EL_EINVAL;
    direct_reply(algorithm_info_2_json_str(&algorithm_info));
}

}  // namespace sscma::callback
