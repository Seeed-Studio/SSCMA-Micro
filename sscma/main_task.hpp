#pragma once

#include "sscma/callback/action.hpp"
#include "sscma/callback/algorithm.hpp"
#include "sscma/callback/default.hpp"
#include "sscma/callback/info.hpp"
#include "sscma/callback/invoke.hpp"
#include "sscma/callback/model.hpp"
#include "sscma/callback/sample.hpp"
#include "sscma/callback/sensor.hpp"
#include "sscma/static_resourse.hpp"
#include "sscma/types.hpp"

namespace sscma::main_task {

using namespace sscma::types;
using namespace sscma::callback;

void run() {
    static_resourse->instance->register_cmd("HELP?", "List available commands", "", [&](std::vector<std::string> argv) {
        const auto& registered_cmds = static_resourse->instance->get_registered_cmds();
        print_help(registered_cmds);
        return EL_OK;
    });

    static_resourse->instance->register_cmd(
      "ID?", "Get device ID", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_device_id(argv[0]);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "NAME?", "Get device name", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_device_name(argv[0]);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "STAT?", "Get device status", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_device_status(argv[0]);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "VER?", "Get version details", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_version(argv[0]);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "RST", "Reboot device", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          static_resourse->executor->add_task(
            [&](std::atomic<bool>& stop_token) { static_resourse->device->restart(); });
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "BREAK", "Stop all running tasks", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          static_resourse->executor->add_task(
            [cmd = std::string(argv[0])](std::atomic<bool>& stop_token) { break_task(cmd); });
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "YIELD", "Yield for 10ms", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          vTaskDelay(10 / portTICK_PERIOD_MS);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "LED", "Set LED status", "ENABLE/DISABLE", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          el_status_led(std::atoi(argv[1].c_str()) ? true : false);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "MODELS?", "Get available models", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_available_models(argv[0]);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "MODEL", "Load a model by model ID", "MODEL_ID", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          uint8_t model_id = std::atoi(argv[1].c_str());
          static_resourse->executor->add_task([&, cmd = std::string(argv[0]), model_id = std::move(model_id)](
                                                std::atomic<bool>& stop_token) { set_model(cmd, model_id); });
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "MODEL?", "Get current model info", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_model_info(argv[0]);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "ALGOS?", "Get available algorithms", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_available_algorithms(argv[0]);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "ALGO", "Set a algorithm by algorithm type ID", "ALGORITHM_ID", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          el_algorithm_type_t algorithm_type = static_cast<el_algorithm_type_t>(std::atoi(argv[1].c_str()));
          static_resourse->executor->add_task(
            [&, cmd = std::string(argv[0]), algorithm_type = std::move(algorithm_type)](std::atomic<bool>& stop_token) {
                set_algorithm(cmd, algorithm_type);
            });
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "ALGO?", "Get current algorithm info", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_algorithm_info(argv[0]);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "SENSORS?", "Get available sensors", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_available_sensors(argv[0]);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "SENSOR",
      "Set a default sensor by sensor ID",
      "SENSOR_ID,ENABLE/DISABLE",
      repl_cmd_cb_t([&](std::vector<std::string> argv) {
          uint8_t sensor_id = std::atoi(argv[1].c_str());
          bool    enable    = std::atoi(argv[2].c_str()) ? true : false;
          static_resourse->executor->add_task(
            [&, cmd = std::string(argv[0]), sensor_id = std::move(sensor_id), enable = std::move(enable)](
              std::atomic<bool>& stop_token) { set_sensor(cmd, sensor_id, enable); });
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "SENSOR?", "Get current sensor info", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_sensor_info(argv[0]);
          return EL_OK;
      }));

    // TODO: sensor config command

    static_resourse->instance->register_cmd(
      "SAMPLE", "Sample data from current sensor", "N_TIMES", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          int n_times = std::atoi(argv[1].c_str());
          static_resourse->executor->add_task(
            [&, cmd = std::string(argv[0]), n_times = std::move(n_times)](std::atomic<bool>& stop_token) {
                run_sample(cmd, n_times, stop_token);
            });
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "SAMPLE?", "Get sample task status", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          task_status(argv[0], static_resourse->is_sample);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "INVOKE",
      "Invoke for N times (-1 for infinity loop)",
      "N_TIMES,RESULT_ONLY",
      repl_cmd_cb_t([&](std::vector<std::string> argv) {
          int  n_times     = std::atoi(argv[1].c_str());
          bool result_only = std::atoi(argv[2].c_str()) ? true : false;
          static_resourse->executor->add_task(
            [&, cmd = std::string(argv[0]), n_times = std::move(n_times), result_only = std::move(result_only)](
              std::atomic<bool>& stop_token) mutable { run_invoke(cmd, n_times, result_only, stop_token); });
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "INVOKE?", "Get invoke task status", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          task_status(argv[0], static_resourse->is_invoke);
          return EL_OK;
      }));

    // Note:
    //    AT+ACTION="count(id,0)>=3","LED=1","LED=0","YIELD"
    //    AT+ACTION="max_score(target,0)>=80","LED=1","LED=0","YIELD"
    //    AT+ACTION="(count(target,0)>=3)||(max_score(target,0)>=80)","LED=1","LED=0","YIELD"
    static_resourse->instance->register_cmd("ACTION",
                                            "Set action trigger",
                                            "\"COND\",\"TRUE_CMD\",\"FALSE_CMD\", \"EXCEPTION_CMD\"",
                                            repl_cmd_cb_t([&](std::vector<std::string> argv) {
                                                set_action(argv);
                                                return EL_OK;
                                            }));

    static_resourse->instance->register_cmd(
      "ACTION?", "Get action trigger", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_action(argv[0]);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "INFO", "Store info string to device flash", "\"INFO_STRING\"", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          set_info(argv);
          return EL_OK;
      }));

    static_resourse->instance->register_cmd(
      "INFO?", "Get info string from device flash", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          get_info(argv[0]);
          return EL_OK;
      }));

    // start task executor
    static_resourse->executor->start();

    // setup components
    {
        std::string cmd;
        if (static_resourse->current_model_id) {
            cmd = std::string("AT+MODEL=") + std::to_string(static_resourse->current_model_id);
            static_resourse->instance->exec(cmd);
        }
        if (static_resourse->current_sensor_id) {
            cmd = std::string("AT+SENSOR=") + std::to_string(static_resourse->current_sensor_id) + ",1";
            static_resourse->instance->exec(cmd);
        }
        if (static_resourse->storage->contains("edgelab_action")) {
            char action[CMD_MAX_LENGTH]{};
            *static_resourse->storage >> el_make_storage_kv("edgelab_action", action);
            static_resourse->instance->exec(action_str_2_cmd_str(action));
        }
        static_resourse->is_ready.store(true);
    }

    // enter service loop (TODO: pipeline builder)
    char* buf = new char[CMD_MAX_LENGTH + sizeof(int)]{};
    for (;;) {
        static_resourse->transport->get_line(buf, CMD_MAX_LENGTH);
        static_resourse->instance->exec(buf);
    }
    delete[] buf;
}

}  // namespace sscma::main_task
