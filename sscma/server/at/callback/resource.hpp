#pragma once

#include <core/ma_config_internal.h>
#include <core/ma_core.h>
#include <porting/ma_porting.h>

#include <atomic>

using namespace ma;

namespace ma::server::callback {

using namespace ma::engine;

struct StaticResource final {
   private:
    StaticResource() {
        device = Device::getInstance();
#ifdef MA_USE_ENGINE_TFLITE
        static EngineTFLite engine_default;
#elif MA_USE_ENGINE_CVI
        static EngineCVI engine_default;
#else
    #error "No engine is defined"
#endif
        engine  = &engine_default;
        int ret = engine->init();
        if (ret != MA_OK) {
            MA_LOGE(MA_TAG, "Engine init failed: %d", ret);
        }

        static Executor executor_default;
        executor = &executor_default;

        current_task_id = 0;

        is_ready  = false;
        is_sample = false;
        is_invoke = false;
    }

   public:
    static StaticResource* getInstance() noexcept {
        static StaticResource instance;
        return &instance;
    }

    Device*   device   = nullptr;
    Engine*   engine   = nullptr;
    Executor* executor = nullptr;

    std::atomic<std::size_t> current_task_id  = 0;
    size_t                   current_model_id = 0;
    size_t                   current_sensor_id = 0;

    std::atomic<bool> is_ready  = false;
    std::atomic<bool> is_sample = false;
    std::atomic<bool> is_invoke = false;
};

#define static_resource StaticResource::getInstance()

}  // namespace ma::server::callback