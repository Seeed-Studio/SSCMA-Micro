#pragma once

#include <algorithm>
#include <forward_list>

#include "callback/action.hpp"
#include "callback/algorithm.hpp"
#include "callback/common.hpp"
#include "callback/info.hpp"
#include "callback/invoke.hpp"
#include "callback/model.hpp"
#include "callback/mqtt.hpp"
#include "callback/sample.hpp"
#include "callback/sensor.hpp"
#include "callback/wifi.hpp"
#include "hooks.hpp"
#include "static_resource.hpp"
#include "types.hpp"

namespace sscma::main_task {

using namespace sscma::types;
using namespace sscma::callback;
using namespace sscma::hooks;

void init_static_resource();
void register_commands();
void wait_for_inputs();

void run() {
    init_static_resource();
    register_commands();
    wait_for_inputs();
}

void init_static_resource() {
    static_resource->init([]() {
        EL_LOGI("[SSCMA] running post init...");
        static_resource->executor->add_task([](const std::atomic<bool>&) {
            std::string caller{"INIT"};
            init_algorithm_hook(caller);
            init_model_hook(caller);
            init_sensor_hook(caller);
            init_action_hook(caller);
            init_wifi_hook(caller);
            init_mqtt_hook(caller);
        });
    });
}

void register_commands() {
    EL_LOGI("[SSCMA] registering AT commands...");

    static_resource->instance->register_cmd(
      "HELP?", "List available commands", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task([caller](const std::atomic<bool>&) {
              print_help(static_resource->instance->get_registered_cmds(), caller);
          });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "ID?", "Get device ID", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_device_id(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "NAME?", "Get device name", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_device_name(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "STAT?", "Get device status", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_device_status(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "VER?", "Get version details", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_version(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd("RST", "Reboot device", "", [](std::vector<std::string>, void*) {
        static_resource->executor->add_task([](const std::atomic<bool>&) { static_resource->device->reset(); });
        return EL_OK;
    });

    static_resource->instance->register_cmd(
      "BREAK", "Stop all running tasks", "", [](std::vector<std::string> argv, void* caller) {
          if (static_resource->is_ready.load()) [[likely]]
              static_resource->executor->try_stop_task();
          static_resource->executor->add_task([cmd = std::move(argv[0]), caller](const std::atomic<bool>&) {
              static_resource->current_task_id.fetch_add(1);
              break_task(cmd, caller);
          });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "YIELD", "Yield running tasks for a period time", "TIME_S", [](std::vector<std::string> argv, void*) {
          static_resource->executor->add_task([cmd = std::move(argv[0]), period = std::atoi(argv[1].c_str())](
                                                const std::atomic<bool>& stop_token) mutable {
              while (stop_token.load(std::memory_order_seq_cst) && period > 0) {
                  el_sleep(1000);
                  period -= 1;
              }
          });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "LED", "Set LED status", "ENABLE/DISABLE", [](std::vector<std::string> argv, void*) {
          el_status_led(std::atoi(argv[1].c_str()));
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "MODELS?", "Get available models", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_available_models(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "MODEL", "Load a model by model ID", "MODEL_ID", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), model_id = std::atoi(argv[1].c_str()), caller](const std::atomic<bool>&) {
                static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
                set_model(cmd, model_id, caller);
            });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "MODEL?", "Get current model info", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_model_info(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "ALGOS?", "Get available algorithms", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_available_algorithms(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "ALGO", "Set a algorithm by algorithm type ID", "ALGORITHM_ID", [](std::vector<std::string> argv, void* caller) {
          el_algorithm_type_t algorithm_type = static_cast<el_algorithm_type_t>(std::atoi(argv[1].c_str()));
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), algorithm_type = std::move(algorithm_type), caller](const std::atomic<bool>&) {
                static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
                set_algorithm(cmd, algorithm_type, caller);
            });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "ALGO?", "Get current algorithm info", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_algorithm_info(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "SENSORS?", "Get available sensors", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_available_sensors(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "SENSOR",
      "Set a default sensor by sensor ID",
      "SENSOR_ID,ENABLE/DISABLE",
      [](std::vector<std::string> argv, void* caller) {
          uint8_t sensor_id = std::atoi(argv[1].c_str());
          bool    enable    = std::atoi(argv[2].c_str()) ? true : false;
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), sensor_id = std::move(sensor_id), enable = std::move(enable), caller](
              const std::atomic<bool>&) {
                static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
                set_sensor(cmd, sensor_id, enable, caller);
            });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "SENSOR?", "Get current sensor info", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_sensor_info(cmd, caller); });
          return EL_OK;
      });

    // TODO: sensor config command

    static_resource->instance->register_cmd(
      "SAMPLE", "Sample data from current sensor", "N_TIMES", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), n_times = std::atoi(argv[1].c_str()), caller](const std::atomic<bool>&) {
                static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
                Sample::create(cmd, n_times, caller)->run();
            });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "SAMPLE?", "Get sample task status", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task([cmd = std::move(argv[0]), caller](const std::atomic<bool>&) {
              task_status(cmd, static_resource->is_sample, caller);
          });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "INVOKE",
      "Invoke for N times (-1 for infinity loop)",
      "N_TIMES,DIFFERED,RESULT_ONLY",
      [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task([cmd         = std::move(argv[0]),
                                               n_times     = std::atoi(argv[1].c_str()),
                                               differed    = std::atoi(argv[2].c_str()),
                                               result_only = std::atoi(argv[3].c_str()),
                                               caller](const std::atomic<bool>&) {
              static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
              Invoke::create(cmd, n_times, differed != 0, result_only != 0, caller)->run();
          });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "INVOKE?", "Get invoke task status", "", [&](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task([cmd = std::move(argv[0]), caller](const std::atomic<bool>&) {
              task_status(cmd, static_resource->is_invoke, caller);
          });
          return EL_OK;
      });

    // Note:
    //    AT+ACTION="((count(target,0)>=3)&&led(1))||led(0)"
    //    AT+ACTION="((max_score(target,0)>=80)&&led(1))||led(0)"
    //    AT+ACTION="(((count(target,0)>=3)||(max_score(target,0)>=80))&&led(1))||led(0)"
    static_resource->instance->register_cmd(
      "ACTION", "Set action trigger", "\"EXPRESSION\"", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [argv = std::move(argv), caller](const std::atomic<bool>&) { set_action(argv, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "ACTION?", "Get action trigger", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_action(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "INFO", "Store info string to device flash", "\"INFO_STRING\"", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [argv = std::move(argv), caller](const std::atomic<bool>&) { set_info(argv, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "INFO?", "Get info string from device flash", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_info(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "WIFI",
      "Set and connect to a Wi-Fi",
      "\"NAME\",SECURITY,\"PASSWORD\"",
      [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [argv = std::move(argv), caller](const std::atomic<bool>&) { set_wifi_network(argv, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "WIFI?", "Get current Wi-Fi status and config", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_wifi_network(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "MQTTSERVER",
      "Set and connect to a MQTT server",
      "\"CLIENT_ID\",\"ADDRESS\",PORT,\"USERNAME\",\"PASSWORD\",USE_SSL",
      [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [argv = std::move(argv), caller](const std::atomic<bool>&) { set_mqtt_server(argv, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "MQTTSERVER?", "Get current MQTT server status and config", "", [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_mqtt_server(cmd, caller); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "MQTTPUBSUB?",
      "Get current MQTT publish and subscribe topic",
      "",
      [](std::vector<std::string> argv, void* caller) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), caller](const std::atomic<bool>&) { get_mqtt_pubsub(cmd, caller); });
          return EL_OK;
      });
}

// void print_memory_usage() {
//     static uint64_t last_time = 0;

//     auto now = el_get_time_ms();
//     if (now - last_time < 2000) {
//         return;
//     }

//     last_time = now;

//     size_t freeHeapSize = xPortGetFreeHeapSize();
//     printf("Free heap memory: %u bytes\n", freeHeapSize);
// }

void wait_for_inputs() {
    // mark the system status as ready
    static_resource->executor->add_task(
        [](const std::atomic<bool>&) { static_resource->is_ready.store(true); }
    );

    EL_LOGI("[SSCMA] AT server is ready to use :)");

    auto transports = std::forward_list<Transport*>{
        static_resource->serial,
        static_resource->mqtt,
        static_resource->wire
    };
    char* buf = reinterpret_cast<char*>(el_aligned_malloc_once(16, SSCMA_CMD_MAX_LENGTH + 1));
    std::memset(buf, 0, SSCMA_CMD_MAX_LENGTH + 1);

Loop:
    std::for_each(transports.begin(), transports.end(), [&buf](Transport* transport) {
        if (transport && *transport && transport->get_line(buf, SSCMA_CMD_MAX_LENGTH)) {
            static_resource->instance->exec(buf, static_cast<void*>(transport));
            std::memset(buf, 0, SSCMA_CMD_MAX_LENGTH + 1);
        }
    });
    // print_memory_usage();
    el_sleep(20);
    goto Loop;
}

}  // namespace sscma::main_task
