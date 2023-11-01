#pragma once

#include <atomic>
#include <cstdbool>
#include <cstdint>
#include <string>

#include "core/algorithm/el_algorithm_delegate.h"
#include "core/data/el_data_models.h"
#include "core/data/el_data_storage.hpp"
#include "core/el_common.h"
#include "core/engine/el_engine_tflite.h"
#include "core/utils/el_hash.h"
#include "porting/el_device.h"
#include "sscma/interpreter/condition.hpp"
#include "sscma/repl/executor.hpp"
#include "sscma/repl/server.hpp"
#include "sscma/utility.hpp"

namespace sscma {

using namespace edgelab;
using namespace edgelab::base;

using namespace sscma::utility;
using namespace sscma::interpreter;
using namespace sscma::repl;

class StaticResource {
   public:
    // library features
    Server*    instance;
    Executor*  executor;
    Condition* action;

    // internal status
    int32_t boot_count;

    uint8_t             current_model_id;
    el_algorithm_type_t current_algorithm_type;
    uint8_t             current_sensor_id;

    std::atomic<size_t> current_task_id;
    std::atomic<bool>   is_ready;
    std::atomic<bool>   is_sample;
    std::atomic<bool>   is_invoke;

    // external resources (hardware related)
    Device*            device;
    Transport*         transport;
    Models*            models;
    Storage*           storage;
    Engine*            engine;
    AlgorithmDelegate* algorithm_delegate;

    // destructor
    ~StaticResource() = default;

    // initializer
    static StaticResource* get_static_resource() {
        static StaticResource static_resource{};
        return &static_resource;
    }

   protected:
    StaticResource() {
        static auto v_instance{Server()};
        instance = &v_instance;
        static auto v_executor{Executor()};
        executor = &v_executor;
        static auto v_action{Condition()};
        action = &v_action;

        boot_count = 0;

        current_model_id       = 1;
        current_algorithm_type = EL_ALGO_TYPE_UNDEFINED;
        current_sensor_id      = 1;

        current_task_id = 0;
        is_ready        = false;
        is_sample       = false;
        is_invoke       = false;

        device    = Device::get_device();
        transport = device->get_transport();
        static auto v_models{Models()};
        models = &v_models;
        static auto v_storage{Storage()};
        storage = &v_storage;
        static auto v_engine{EngineTFLite()};
        engine             = &v_engine;
        algorithm_delegate = AlgorithmDelegate::get_delegate();

        inter_init();
    }

    inline void inter_init() {
        init_hardware();
        init_backend();
        init_frontend();
    }

    inline void init_hardware() { transport->init(); }

    inline void init_backend() {
        models->init();
        storage->init();

        if (!storage->contains(SSCMA_STORAGE_KEY_VERSION) || [this]() -> bool {
                char version[EL_VERSION_LENTH_MAX]{};
                *this->storage >> el_make_storage_kv(SSCMA_STORAGE_KEY_VERSION, version);
                return std::string(EL_VERSION) != version;
            }())
            *storage << el_make_storage_kv(SSCMA_STORAGE_KEY_VERSION, EL_VERSION)
                     << el_make_storage_kv("current_model_id", current_model_id)
                     << el_make_storage_kv("current_algorithm_type", current_algorithm_type)
                     << el_make_storage_kv("current_sensor_id", current_sensor_id)
                     << el_make_storage_kv("boot_count", boot_count);

        *storage >> el_make_storage_kv("current_model_id", current_model_id) >>
          el_make_storage_kv("current_algorithm_type", current_algorithm_type) >>
          el_make_storage_kv("current_sensor_id", current_sensor_id) >> el_make_storage_kv("boot_count", boot_count);

        *storage << el_make_storage_kv("boot_count", ++boot_count);
    }

    inline void init_frontend() {
        instance->init([this](el_err_code_t ret, std::string msg) {
            if (ret != EL_OK) [[unlikely]] {
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
