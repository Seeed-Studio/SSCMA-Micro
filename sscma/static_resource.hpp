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
#include "interpreter/condition.hpp"
#include "porting/el_device.h"
#include "repl/executor.hpp"
#include "repl/server.hpp"
#include "repl/supervisor.hpp"
#include "utility.hpp"

#if SSCMA_HAS_NATIVE_NETWORKING
    #include "interface/transport/mqtt.hpp"
    #include "interface/wifi.hpp"
#endif

namespace sscma {

using namespace edgelab;
using namespace edgelab::base;

using namespace sscma::types;
using namespace sscma::utility;
using namespace sscma::repl;
using namespace sscma::interpreter;

#if SSCMA_HAS_NATIVE_NETWORKING
using namespace sscma::interface;
using namespace sscma::transport;
#endif

class StaticResource final {
   public:
    // SSCMA library feature handlers
    Server*     instance;
    Executor*   executor;
    Supervisor* supervisor;
#if SSCMA_CFG_ENABLE_ACTION
    Condition* action;
#endif

    // internal configs that stored in flash
    int32_t             boot_count;
    uint8_t             current_model_id;
    uint8_t             current_sensor_id;
    uint16_t            current_sensor_opt;
    el_algorithm_type_t current_algorithm_type;

    // internal states
    std::atomic<std::size_t> current_task_id;
    std::atomic<bool>        is_ready;
    bool                     is_sample;
    bool                     is_invoke;

    // external resources (hardware related)
    Device*                       device;
    std::forward_list<Transport*> transports;

#if SSCMA_HAS_NATIVE_NETWORKING
    WiFi* wifi;
    MQTT* mqtt;
#endif

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

        transports = device->get_transports();

        models             = Models::get_ptr();
        storage            = Storage::get_ptr();
        algorithm_delegate = AlgorithmDelegate::get_ptr();

        static auto v_engine{EngineTFLite()};
        engine = &v_engine;

#if SSCMA_HAS_NATIVE_NETWORKING
        static auto v_wifi{WiFi()};
        wifi = &v_wifi;

        static auto v_mqtt{MQTT(wifi)};
        mqtt       = &v_mqtt;
        mqtt->type = EL_TRANSPORT_MQTT;
#endif

        static auto v_instance{Server()};
        instance = &v_instance;

        static auto v_executor{Executor(SSCMA_REPL_EXECUTOR_STACK_SIZE, SSCMA_REPL_EXECUTOR_PRIO)};
        executor = &v_executor;

        supervisor = Supervisor::get_ptr();

#if SSCMA_CFG_ENABLE_ACTION
        static auto v_action{Condition()};
        action = &v_action;
#endif
        inter_init();

        if (post_init) post_init();
    }

   protected:
    StaticResource() = default;

    inline void inter_init() {
        EL_LOGI("[SSCMA] internal initializing begin...");
        boot_count             = 0;
        current_model_id       = 1;
        current_sensor_id      = 1;
        current_sensor_opt     = 1;
        current_algorithm_type = EL_ALGO_TYPE_UNDEFINED;

        current_task_id = 0;
        is_ready        = false;
        is_sample       = false;
        is_invoke       = false;

        init_hardware();
        init_backend();
        init_frontend();
    }

    inline void init_hardware() {
        EL_LOGI("[SSCMA] initializing basic IO devices...");
        // TODO: init them inside device manager
        for (auto& transport : transports) {
            if (!transport) [[unlikely]]
                continue;

            switch (transport->type) {
            case EL_TRANSPORT_UART:
                reinterpret_cast<Serial*>(transport)->init();
                break;
            case EL_TRANSPORT_SPI:
#if !SSCMA_HAS_NATIVE_NETWORKING
                reinterpret_cast<SSPI*>(transport)->init();
#endif
                break;
            case EL_TRANSPORT_I2C:
                reinterpret_cast<Wire*>(transport)->init();
                break;
            case EL_TRANSPORT_MQTT:
            default:
                break;
            }
        }
    }

    inline void init_backend() {
        EL_LOGI("[SSCMA] loading resources from flash...");
        models->init();
        storage->init();

        char version[SSCMA_STORAGE_VERSION_LENGTH_MAX]{};
        auto kv = el_make_storage_kv(SSCMA_STORAGE_KEY_VERSION, version);
        // if version match, load other configs from storage
        if (storage->get(kv) && std::strcmp(kv.value, SSCMA_STORAGE_VERSION) == 0) [[likely]]
            *storage >> el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_MODEL_ID, current_model_id) >>
              el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_SENSOR_ID, current_sensor_id) >>
              el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_SENSOR_OPT, current_sensor_opt) >>
              el_make_storage_kv_from_type(current_algorithm_type) >>
              el_make_storage_kv(SSCMA_STORAGE_KEY_BOOT_COUNT, boot_count);
        else {  // else init flash storage
            std::strncpy(version, SSCMA_STORAGE_VERSION, sizeof(version));
            *storage << kv << el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_MODEL_ID, current_model_id)
                     << el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_SENSOR_ID, current_sensor_id)
                     << el_make_storage_kv(SSCMA_STORAGE_KEY_CONF_SENSOR_OPT, current_sensor_opt)
                     << el_make_storage_kv_from_type(current_algorithm_type)
                     << el_make_storage_kv(SSCMA_STORAGE_KEY_BOOT_COUNT, boot_count);
        }

#if SSCMA_STORAGE_CFG_UPDATE_BOOT_COUNT
        // increment boot count
        *storage << el_make_storage_kv(SSCMA_STORAGE_KEY_BOOT_COUNT, ++boot_count);
#endif
    }

    inline void init_frontend() {
        EL_LOGI("[SSCMA] initializing AT server...");
        // init AT server
        instance->init([](void* caller, el_err_code_t ret, std::string msg) {  // server print callback function
            if (ret != EL_OK) [[unlikely]] {  // only send error message when error occurs
                msg.erase(std::remove_if(msg.begin(), msg.end(), [](char c) { return std::iscntrl(c); }), msg.end());
                auto ss{concat_strings("\r{\"type\": 2, \"name\": \"AT\", \"code\": ",
                                       std::to_string(ret),
                                       ", \"data\": ",
                                       quoted(msg),
                                       "}\n")};
                if (caller) static_cast<Transport*>(caller)->send_bytes(ss.c_str(), ss.size());
            }
        });
    }
};

static auto* static_resource{StaticResource::get_static_resource()};

}  // namespace sscma
