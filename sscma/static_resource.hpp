#pragma once

#include <atomic>
#include <cstdbool>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

#include "core/algorithm/el_algorithm_delegate.h"
#include "core/data/el_data_models.h"
#include "core/data/el_data_storage.hpp"
#include "core/el_common.h"
#include "core/engine/el_engine_tflite.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "core/utils/el_hash.h"
#include "extension/mux_transport.hpp"
#include "porting/el_device.h"
#include "sscma/interpreter/condition.hpp"
#include "sscma/repl/executor.hpp"
#include "sscma/repl/server.hpp"
#include "sscma/utility.hpp"

namespace sscma {

using namespace edgelab;
using namespace edgelab::base;

using namespace sscma::extension;
using namespace sscma::repl;
using namespace sscma::interpreter;
using namespace sscma::types;
using namespace sscma::utility;

class StaticResource final {
   public:
    // SSCMA library feature handlers
    Server*    instance;
    Executor*  executor;
    Condition* action;

    // internal configs that stored in flash
    int32_t              boot_count;
    uint8_t              current_model_id;
    uint8_t              current_sensor_id;
    el_algorithm_type_t  current_algorithm_type;
    mqtt_pubsub_config_t current_mqtt_pubsub_config;

    // internal states
    std::atomic<size_t> current_task_id;
    std::atomic<bool>   is_ready;
    bool                is_sample;
    bool                is_invoke;

    // external states
    std::atomic<bool> enable_network_supervisor;
    el_net_sta_t      target_network_status;

    // external resources (hardware related)
    Device*            device;
    Serial*            serial;
    Network*           network;
    MuxTransport*      transport;
    Models*            models;
    Storage*           storage;
    Engine*            engine;
    AlgorithmDelegate* algorithm_delegate;

    // destructor
    ~StaticResource() = default;

    // static constructor (on stack)
    static inline StaticResource* get_static_resource() {
        static StaticResource static_resource{};
        return &static_resource;
    }

    void init(std::function<void(void)> post_init) {
        device = Device::get_device();
        // Important: init device first before using it (serial, network, etc.)
        device->init();

        serial    = device->get_serial();
        network   = device->get_network();
        transport = MuxTransport::get_transport();

        static auto v_instance{Server()};
        instance = &v_instance;

        static auto v_executor{Executor()};
        executor = &v_executor;

        static auto v_action{Condition()};
        action = &v_action;

        static auto v_models{Models()};
        models = &v_models;

        static auto v_storage{Storage()};
        storage = &v_storage;

        static auto v_engine{EngineTFLite()};
        engine = &v_engine;

        algorithm_delegate = AlgorithmDelegate::get_delegate();

        inter_init();

        if (post_init) post_init();
    }

   protected:
    StaticResource() = default;

    inline void inter_init() {
        boot_count                 = 0;
        current_model_id           = 1;
        current_sensor_id          = 1;
        current_algorithm_type     = EL_ALGO_TYPE_UNDEFINED;
        current_mqtt_pubsub_config = get_default_mqtt_pubsub_config(device);

        current_task_id = 0;
        is_ready        = false;
        is_sample       = false;
        is_invoke       = false;

        init_hardware();
        init_backend();
        init_frontend();
    }

    inline void init_hardware() {
        serial->init();
        network->init();
        transport->init();
    }

    inline void init_backend() {
        models->init();
        storage->init();

        char version[EL_VERSION_LENTH_MAX]{};
        auto kv = el_make_storage_kv(SSCMA_STORAGE_KEY_VERSION, version);
        // if version match, load other configs from storage
        if (storage->get(kv) && std::strcmp(kv.value, EL_VERSION) == 0) [[likely]]
            *storage >> el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_MODEL_ID, current_model_id) >>
              el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_SENSOR_ID, current_sensor_id) >>
              el_make_storage_kv_from_type(current_algorithm_type) >>
              el_make_storage_kv_from_type(current_mqtt_pubsub_config) >>
              el_make_storage_kv(SSCMA_STORAGE_KEY_BOOT_COUNT, boot_count);
        else {  // else init flash storage
            std::strncpy(version, EL_VERSION, sizeof(version));
            *storage << kv << el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_MODEL_ID, current_model_id)
                     << el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_SENSOR_ID, current_sensor_id)
                     << el_make_storage_kv_from_type(current_algorithm_type)
                     << el_make_storage_kv_from_type(current_mqtt_pubsub_config)
                     << el_make_storage_kv(SSCMA_STORAGE_KEY_BOOT_COUNT, boot_count);
        }

        // increment boot count
        *storage << el_make_storage_kv(SSCMA_STORAGE_KEY_BOOT_COUNT, ++boot_count);

        // network supervisor
        enable_network_supervisor = false;
        target_network_status     = [this]() {
            auto target_status = this->network->status();
            {
                auto config = wireless_network_config_t{};
                if (this->storage->get(el_make_storage_kv_from_type(config))) {
                    if (std::strlen(config.name)) target_status = NETWORK_JOINED;
                } else
                    return target_status;
            }
            {
                auto config = mqtt_server_config_t{};
                if (this->storage->get(el_make_storage_kv_from_type(config))) {
                    if (std::strlen(config.address)) target_status = NETWORK_CONNECTED;
                } else
                    return target_status;
            }
            return target_status;
        }();
    }

    inline void init_frontend() {
        // init AT server
        instance->init([this](el_err_code_t ret, std::string msg) {  // server print callback function
            if (ret != EL_OK) [[unlikely]] {                         // only send error message when error occurs
                msg.erase(std::remove_if(msg.begin(), msg.end(), [](char c) { return std::iscntrl(c); }), msg.end());
                const auto& ss{concat_strings("\r{\"type\": 2, \"name\": \"AT\", \"code\": ",
                                              std::to_string(ret),
                                              ", \"data\": ",
                                              quoted(msg),
                                              "}\n")};
                this->transport->send_bytes(ss.c_str(), ss.size());
            }
        });
    }
};

static auto* static_resource{StaticResource::get_static_resource()};

}  // namespace sscma
