#pragma once

#include "sscma/callback/action.hpp"
#include "sscma/callback/algorithm.hpp"
#include "sscma/callback/common.hpp"
#include "sscma/callback/info.hpp"
#include "sscma/callback/invoke.hpp"
#include "sscma/callback/model.hpp"
#include "sscma/callback/mqtt.hpp"
#include "sscma/callback/network.hpp"
#include "sscma/callback/sample.hpp"
#include "sscma/callback/sensor.hpp"
#include "sscma/static_resource.hpp"
#include "sscma/types.hpp"

namespace sscma::main_task {

using namespace sscma::types;
using namespace sscma::callback;

void run() {
    // init static_resource
    // Important: init device first before using it (serial, network, etc.)
    static_resource->init();

    // register commands
    static_resource->instance->register_cmd("HELP?", "List available commands", "", [](std::vector<std::string> argv) {
        print_help(static_resource->instance->get_registered_cmds());
        return EL_OK;
    });

    static_resource->instance->register_cmd("ID?", "Get device ID", "", [](std::vector<std::string> argv) {
        get_device_id(argv[0]);
        return EL_OK;
    });

    static_resource->instance->register_cmd("NAME?", "Get device name", "", [](std::vector<std::string> argv) {
        get_device_name(argv[0]);
        return EL_OK;
    });

    static_resource->instance->register_cmd("STAT?", "Get device status", "", [](std::vector<std::string> argv) {
        get_device_status(argv[0]);
        return EL_OK;
    });

    static_resource->instance->register_cmd("VER?", "Get version details", "", [](std::vector<std::string> argv) {
        get_version(argv[0]);
        return EL_OK;
    });

    static_resource->instance->register_cmd("RST", "Reboot device", "", [](std::vector<std::string> argv) {
        static_resource->executor->add_task([](const std::atomic<bool>&) { static_resource->device->reset(); });
        return EL_OK;
    });

    static_resource->instance->register_cmd("BREAK", "Stop all running tasks", "", [](std::vector<std::string> argv) {
        static_resource->executor->try_stop_task();
        static_resource->executor->add_task([cmd = std::move(argv[0])](const std::atomic<bool>&) {
            static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
            break_task(cmd);
        });
        return EL_OK;
    });

    static_resource->instance->register_cmd(
      "YIELD", "Yield running tasks for a period time", "TIME_S", [](std::vector<std::string> argv) {
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
      "LED", "Set LED status", "ENABLE/DISABLE", [](std::vector<std::string> argv) {
          el_status_led(std::atoi(argv[1].c_str()) ? true : false);
          return EL_OK;
      });

    static_resource->instance->register_cmd("MODELS?", "Get available models", "", [](std::vector<std::string> argv) {
        static_resource->executor->add_task(
          [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_available_models(cmd); });
        return EL_OK;
    });

    static_resource->instance->register_cmd(
      "MODEL", "Load a model by model ID", "MODEL_ID", [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), model_id = std::atoi(argv[1].c_str())](const std::atomic<bool>&) {
                static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
                set_model(cmd, model_id);
            });
          return EL_OK;
      });

    static_resource->instance->register_cmd("MODEL?", "Get current model info", "", [](std::vector<std::string> argv) {
        static_resource->executor->add_task(
          [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_model_info(cmd); });
        return EL_OK;
    });

    static_resource->instance->register_cmd(
      "ALGOS?", "Get available algorithms", "", [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_available_algorithms(cmd); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "ALGO", "Set a algorithm by algorithm type ID", "ALGORITHM_ID", [](std::vector<std::string> argv) {
          el_algorithm_type_t algorithm_type = static_cast<el_algorithm_type_t>(std::atoi(argv[1].c_str()));
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), algorithm_type = std::move(algorithm_type)](const std::atomic<bool>&) {
                static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
                set_algorithm(cmd, algorithm_type);
            });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "ALGO?", "Get current algorithm info", "", [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_algorithm_info(cmd); });
          return EL_OK;
      });

    static_resource->instance->register_cmd("SENSORS?", "Get available sensors", "", [](std::vector<std::string> argv) {
        static_resource->executor->add_task(
          [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_available_sensors(cmd); });
        return EL_OK;
    });

    static_resource->instance->register_cmd(
      "SENSOR", "Set a default sensor by sensor ID", "SENSOR_ID,ENABLE/DISABLE", [](std::vector<std::string> argv) {
          uint8_t sensor_id = std::atoi(argv[1].c_str());
          bool    enable    = std::atoi(argv[2].c_str()) ? true : false;
          static_resource->executor->add_task([cmd       = std::move(argv[0]),
                                               sensor_id = std::move(sensor_id),
                                               enable    = std::move(enable)](const std::atomic<bool>&) {
              static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
              set_sensor(cmd, sensor_id, enable);
          });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "SENSOR?", "Get current sensor info", "", [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_sensor_info(cmd); });
          return EL_OK;
      });

    // TODO: sensor config command

    static_resource->instance->register_cmd(
      "SAMPLE", "Sample data from current sensor", "N_TIMES", [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0]), n_times = std::atoi(argv[1].c_str())](const std::atomic<bool>&) {
                static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
                Sample::create(cmd, n_times)->run();
            });
          return EL_OK;
      });

    static_resource->instance->register_cmd("SAMPLE?", "Get sample task status", "", [](std::vector<std::string> argv) {
        static_resource->executor->add_task(
          [cmd = std::move(argv[0])](const std::atomic<bool>&) { task_status(cmd, static_resource->is_sample); });
        return EL_OK;
    });

    static_resource->instance->register_cmd(
      "INVOKE", "Invoke for N times (-1 for infinity loop)", "N_TIMES,RESULT_ONLY", [](std::vector<std::string> argv) {
          static_resource->executor->add_task([cmd         = std::move(argv[0]),
                                               n_times     = std::atoi(argv[1].c_str()),
                                               result_only = std::atoi(argv[2].c_str())](const std::atomic<bool>&) {
              static_resource->current_task_id.fetch_add(1, std::memory_order_seq_cst);
              Invoke::create(cmd, n_times, result_only != 0)->run();
          });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "INVOKE?", "Get invoke task status", "", [&](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { task_status(cmd, static_resource->is_invoke); });
          return EL_OK;
      });

    // Note:
    //    AT+ACTION="((count(target,0)>=3)&&led(1))||led(0)"
    //    AT+ACTION="((max_score(target,0)>=80)&&led(1))||led(0)"
    //    AT+ACTION="(((count(target,0)>=3)||(max_score(target,0)>=80))&&led(1))||led(0)"
    static_resource->instance->register_cmd(
      "ACTION", "Set action trigger", "\"EXPRESSION\"", [](std::vector<std::string> argv) {
          static_resource->executor->add_task([argv = std::move(argv)](const std::atomic<bool>&) { set_action(argv); });
          return EL_OK;
      });

    static_resource->instance->register_cmd("ACTION?", "Get action trigger", "", [](std::vector<std::string> argv) {
        static_resource->executor->add_task([cmd = std::move(argv[0])](const std::atomic<bool>&) { get_action(cmd); });
        return EL_OK;
    });

    static_resource->instance->register_cmd(
      "INFO", "Store info string to device flash", "\"INFO_STRING\"", [](std::vector<std::string> argv) {
          static_resource->executor->add_task([argv = std::move(argv)](const std::atomic<bool>&) { set_info(argv); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "INFO?", "Get info string from device flash", "", [](std::vector<std::string> argv) {
          static_resource->executor->add_task([cmd = std::move(argv[0])](const std::atomic<bool>&) { get_info(cmd); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "WIFI", "Set and connect to a Wi-Fi", "\"NAME\",SECURITY,\"PASSWORD\"", [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [argv = std::move(argv)](const std::atomic<bool>&) { set_wireless_network(argv); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "WIFI?", "Get current Wi-Fi status and config", "", [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_wireless_network(cmd); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "MQTTSERVER",
      "Set and connect to a MQTT server",
      "\"CLIENT_ID\",\"ADDRESS\",\"USERNAME\",\"PASSWORD\",USE_SSL",
      [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [argv = std::move(argv)](const std::atomic<bool>&) { set_mqtt_server(argv); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "MQTTSERVER?", "Get current MQTT server status and config", "", [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_mqtt_server(cmd); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "MQTTPUBSUB",
      "Set the MQTT publish and subscribe topic",
      "\"PUB_TOPIC\",PUB_QOS,\"SUB_TOPIC\",SUB_QOS",
      [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [argv = std::move(argv)](const std::atomic<bool>&) { set_mqtt_pubsub(argv); });
          return EL_OK;
      });

    static_resource->instance->register_cmd(
      "MQTTPUBSUB?", "Get current MQTT publish and subscribe topic", "", [](std::vector<std::string> argv) {
          static_resource->executor->add_task(
            [cmd = std::move(argv[0])](const std::atomic<bool>&) { get_mqtt_pubsub(cmd); });
          return EL_OK;
      });

    // run init tasks
    // set model
    if (static_resource->current_model_id) [[likely]]
        set_model("set_model", static_resource->current_model_id);

    // set sensor
    if (static_resource->current_sensor_id) [[likely]]
        set_sensor("set_sensor", static_resource->current_sensor_id, true);

    // set action
    if (static_resource->storage->contains(SSCMA_STORAGE_KEY_ACTION)) [[likely]] {
        char action[CONFIG_SSCMA_CMD_MAX_LENGTH]{};
        *static_resource->storage >> el_make_storage_kv(SSCMA_STORAGE_KEY_ACTION, action);
        set_action(std::vector<std::string>{"set_action", action});
    }

    // connect to wireless network and setup MQTT
    {
        auto config = wireless_network_config_t{};
        auto kv     = el_make_storage_kv_from_type(config);
        if (static_resource->storage->get(kv)) [[likely]]
            set_wireless_network(std::vector<std::string>{
              "set_wireless_network", config.name, std::to_string(config.security_type), config.passwd});

        if (static_resource->network->status() == NETWORK_JOINED) {
            auto config = mqtt_server_config_t{};
            auto kv     = el_make_storage_kv_from_type(config);
            if (static_resource->storage->get(kv)) [[likely]]
                set_mqtt_server(std::vector<std::string>{"set_mqtt_server",
                                                         config.client_id,
                                                         config.address,
                                                         config.username,
                                                         config.password,
                                                         std::to_string(config.use_ssl ? 1 : 0)});
        }

        if (static_resource->network->status() == NETWORK_CONNECTED) {
            auto config = mqtt_pubsub_config_t{};
            auto kv     = el_make_storage_kv_from_type(config);
            static_resource->storage->get(kv);  // if no config on flash, use default
            set_mqtt_pubsub(std::vector<std::string>{"set_mqtt_pubsub",
                                                     config.pub_topic,
                                                     std::to_string(config.pub_qos),
                                                     config.sub_topic,
                                                     std::to_string(config.sub_qos)});
        }
    }

    // mark the system status as ready
    static_resource->is_ready.store(true);

    // enter service loop
    {
        char* buf = new char[CONFIG_SSCMA_CMD_MAX_LENGTH]{};
    Loop:
        static_resource->transport->get_line(buf, CONFIG_SSCMA_CMD_MAX_LENGTH);
        if (std::strlen(buf) > 1) [[likely]]
            static_resource->instance->exec(buf);
        goto Loop;
    }
}

}  // namespace sscma::main_task
