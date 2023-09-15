#pragma once

#include <atomic>
#include <cstdbool>
#include <cstdint>
#include <sstream>
#include <string>

#include "core/algorithm/el_algorithm_delegate.h"
#include "core/data/el_data_models.h"
#include "core/data/el_data_storage.hpp"
#include "core/el_common.h"
#include "core/engine/el_engine_tflite.h"
#include "core/utils/el_hash.h"
#include "sscma/interpreter/condition.hpp"
#include "sscma/repl/executor.hpp"
#include "sscma/repl/server.hpp"
#include "sscma/utility.hpp"
#include "porting/espressif/el_device_esp.h"

namespace sscma {

using namespace sscma::utility;
using namespace sscma::interpreter;
using namespace sscma::repl;

using namespace edgelab;
using namespace edgelab::base;

class StaticResourse {
   public:
    // external
    Device*            device;
    Transport*         transport;
    Models*            models;
    Storage*           storage;
    AlgorithmDelegate* algorithm_delegate;
    Engine*            engine;

    // lib
    Server*    instance;
    Executor*  executor;
    Condition* action_cond;

    // internal
    int32_t             boot_count;
    uint8_t             current_model_id;
    el_algorithm_type_t current_algorithm_type;
    uint8_t             current_sensor_id;
    std::atomic<bool>   is_ready;
    std::atomic<bool>   is_sample;
    std::atomic<bool>   is_invoke;

    // destructor
    ~StaticResourse() {
        if (engine) [[likely]] {
            delete engine;
            engine = nullptr;
        }

        if (instance) [[likely]] {
            delete instance;
            instance = nullptr;
        }

        if (executor) [[likely]] {
            delete executor;
            executor = nullptr;
        }

        if (action_cond) [[likely]] {
            delete action_cond;
            action_cond = nullptr;
        }
    }

    // initializer
    static StaticResourse* get_static_resource() {
        static StaticResourse static_resourse{};
        return &static_resourse;
    }

   protected:
    StaticResourse() {
        device             = DeviceEsp::get_device();
        transport          = device->get_serial();
        models             = new Models();
        storage            = new Storage();
        algorithm_delegate = AlgorithmDelegate::get_delegate();
        engine             = new EngineTFLite();

        instance    = new Server();
        executor    = new Executor();
        action_cond = new Condition();

        boot_count             = 0;
        current_model_id       = 1;
        current_algorithm_type = EL_ALGO_TYPE_UNDEFINED;
        current_sensor_id      = 1;
        is_ready               = false;
        is_sample              = false;
        is_invoke              = false;

        inter_init();
    }

    void inter_init() {
        transport->init();
        instance->init([this](el_err_code_t ret, const std::string& msg) {
            auto os = std::ostringstream(std::ios_base::ate);
            if (ret != EL_OK)
                os << REPLY_LOG_HEADER << "\"name\": \"AT\", \"code\": " << static_cast<int>(ret)
                   << ", \"data\": " << quoted_stringify(msg) << "}\n";
            const auto& str{os.str()};
            this->transport->send_bytes(str.c_str(), str.size());
        });
        models->init();
        storage->init();

        if (!storage->contains("edgelab") || [&]() -> bool {
                char version[EL_VERSION_LENTH_MAX]{};
                *storage >> el_make_storage_kv("edgelab", version);
                return std::string(EL_VERSION) != version;
            }()) {
            *storage << el_make_storage_kv("edgelab", EL_VERSION)
                     << el_make_storage_kv("current_model_id", current_model_id)
                     << el_make_storage_kv("current_algorithm_type", current_algorithm_type)
                     << el_make_storage_kv("current_sensor_id", current_sensor_id)
                     << el_make_storage_kv("boot_count", boot_count);
        }
        *storage >> el_make_storage_kv("current_model_id", current_model_id) >>
          el_make_storage_kv("current_algorithm_type", current_algorithm_type) >>
          el_make_storage_kv("current_sensor_id", current_sensor_id) >> el_make_storage_kv("boot_count", boot_count);
        *storage << el_make_storage_kv("boot_count", ++boot_count);
    }
};

static auto* static_resourse{StaticResourse::get_static_resource()};

}  // namespace sscma
