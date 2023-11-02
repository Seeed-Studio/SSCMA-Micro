#pragma once

#include "sscma/callback/action.hpp"
#include "sscma/callback/algorithm.hpp"
#include "sscma/callback/common.hpp"
#include "sscma/callback/info.hpp"
#include "sscma/callback/invoke.hpp"
#include "sscma/callback/model.hpp"
#include "sscma/callback/sample.hpp"
#include "sscma/callback/sensor.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/types.hpp"

namespace sscma::main_task {

using namespace sscma::types;
using namespace sscma::callback;

void run() {
    static_resource->init();

    static_resource->instance->register_cmd("HELP?", "List available commands", "", [](std::vector<std::string> argv) {
        const auto& registered_cmds = static_resource->instance->get_registered_cmds();
        print_help(registered_cmds);
        return EL_OK;
    });

    static_resource->instance->register_cmd(
      "ID?", "Get device ID", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          get_device_id(argv[0]);
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "NAME?", "Get device name", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          get_device_name(argv[0]);
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "STAT?", "Get device status", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          get_device_status(argv[0]);
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "VER?", "Get version details", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          get_version(argv[0]);
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "RST", "Reboot device", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task([](const std::atomic<bool>&) { static_resource->device->reset(); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "BREAK", "Stop all running tasks", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->try_stop_task();
          static_resource->executor->add_task([cmd = std::move(argv[0])](const std::atomic<bool>&) {
              static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
              break_task(cmd);
          });

          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "YIELD", "Yield running tasks for a period time", "TIME_S", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task([cmd = std::move(argv[0]), period = std::atoi(argv[1].c_str())](
                                                const std::atomic<bool>& stop_token) mutable {
              while (stop_token.load(std::memory_order_seq_cst) && period > 0) {
                  el_sleep(1000);
                  period -= 1;
              }
          });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "LED", "Set LED status", "ENABLE/DISABLE", repl_cmd_cb_t([](std::vector<std::string> argv) {
          el_status_led(std::atoi(argv[1].c_str()) ? true : false);
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "MODELS?", "Get available models", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_available_models(cmd); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "MODEL", "Load a model by model ID", "MODEL_ID", repl_cmd_cb_t([](std::vector<std::string> argv) {
          uint8_t model_id = std::atoi(argv[1].c_str());
          static_resource->executor->add_task([cmd = std::move(argv[0]), model_id = std::move(model_id)](
                                                const std::atomic<bool>& stop_token) { set_model(cmd, model_id); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "MODEL?", "Get current model info", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_model_info(cmd); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "ALGOS?", "Get available algorithms", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_available_algorithms(cmd); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "ALGO", "Set a algorithm by algorithm type ID", "ALGORITHM_ID", repl_cmd_cb_t([](std::vector<std::string> argv) {
          el_algorithm_type_t algorithm_type = static_cast<el_algorithm_type_t>(std::atoi(argv[1].c_str()));
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), algorithm_type = std::move(algorithm_type)](
              const std::atomic<bool>& stop_token) { set_algorithm(cmd, algorithm_type); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "ALGO?", "Get current algorithm info", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_algorithm_info(cmd); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "SENSORS?", "Get available sensors", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_available_sensors(cmd); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "SENSOR",
      "Set a default sensor by sensor ID",
      "SENSOR_ID,ENABLE/DISABLE",
      repl_cmd_cb_t([](std::vector<std::string> argv) {
          uint8_t sensor_id = std::atoi(argv[1].c_str());
          bool    enable    = std::atoi(argv[2].c_str()) ? true : false;
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), sensor_id = std::move(sensor_id), enable = std::move(enable)](
              const std::atomic<bool>& stop_token) { set_sensor(cmd, sensor_id, enable); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "SENSOR?", "Get current sensor info", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_sensor_info(cmd); });
          return EL_OK;
      }));

    // TODO: sensor config command

    static_resource->instance->register_cmd(
      "SAMPLE", "Sample data from current sensor", "N_TIMES", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
          Sample::create(std::move(argv[0]), std::atoi(argv[1].c_str()))->run();
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "SAMPLE?", "Get sample task status", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          task_status(argv[0], static_resource->is_sample);
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "INVOKE",
      "Invoke for N times (-1 for infinity loop)",
      "N_TIMES,RESULT_ONLY",
      repl_cmd_cb_t([&](std::vector<std::string> argv) {
          static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
          Invoke::create(std::move(argv[0]), std::atoi(argv[1].c_str()), std::atoi(argv[2].c_str()))->run();
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "INVOKE?", "Get invoke task status", "", repl_cmd_cb_t([&](std::vector<std::string> argv) {
          task_status(argv[0], static_resource->is_invoke);
          return EL_OK;
      }));

    // Note:
    //    AT+ACTION="((count(target,0)>=3)&&led(1))||led(0)"
    //    AT+ACTION="((max_score(target,0)>=80)&&led(1))||led(0)"
    //    AT+ACTION="(((count(target,0)>=3)||(max_score(target,0)>=80))&&led(1))||led(0)"
    static_resource->instance->register_cmd(
      "ACTION", "Set action trigger", "\"EXPRESSION\"", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task([argv = std::move(argv)](const std::atomic<bool>&) { set_action(argv); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "ACTION?", "Get action trigger", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_action(cmd); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "INFO", "Store info string to device flash", "\"INFO_STRING\"", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task([argv = std::move(argv)](const std::atomic<bool>&) { set_info(argv); });
          return EL_OK;
      }));

    static_resource->instance->register_cmd(
      "INFO?", "Get info string from device flash", "", repl_cmd_cb_t([](std::vector<std::string> argv) {
          static_resource->executor->add_task([cmd = std::move(argv[0])](const std::atomic<bool>&) { get_info(cmd); });
          return EL_OK;
      }));

    // init commands
    {
        if (static_resource->current_model_id) [[likely]]
            static_resource->instance->exec(
              concat_strings("AT+MODEL=", std::to_string(static_resource->current_model_id)));

        if (static_resource->current_sensor_id) [[likely]]
            static_resource->instance->exec(
              concat_strings("AT+SENSOR=", std::to_string(static_resource->current_sensor_id), ",1"));

        if (static_resource->storage->contains(SSCMA_STORAGE_KEY_ACTION)) [[likely]] {
            char action[CONFIG_SSCMA_CMD_MAX_LENGTH]{};
            *static_resource->storage >> el_make_storage_kv(SSCMA_STORAGE_KEY_ACTION, action);
            static_resource->instance->exec(concat_strings("AT+ACTION=", quoted(action)));
        }

        static_resource->executor->add_task([](const std::atomic<bool>&) { static_resource->is_ready.store(true); });
    }

    // enter service loop
    char* buf = new char[CONFIG_SSCMA_CMD_MAX_LENGTH]{};
    for (;;) {
        static_resource->transport->get_line(buf, CONFIG_SSCMA_CMD_MAX_LENGTH);
        if (std::strlen(buf) > 1) [[likely]]
            static_resource->instance->exec(buf);
    }
    delete[] buf;
}

}  // namespace sscma::main_task
