#pragma once

#include <core/ma_config_internal.h>
#include <core/ma_core.h>
#include <porting/ma_porting.h>

#include <ma_config_board.h>

#include <atomic>

using namespace ma;

namespace ma::server::callback {

using namespace ma::engine;

struct StaticResource final {
   private:
    StaticResource() {
        device = Device::getInstance();

        MA_LOGD(MA_TAG, "Initializing engine");
#if MA_USE_ENGINE_TFLITE
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

        MA_LOGD(MA_TAG, "Initializing executor");
        static Executor executor_default;
        executor = &executor_default;

        MA_STORAGE_GET_POD(device->getStorage(), "ma#score_threshold", shared_threshold_score, shared_threshold_score);
        MA_STORAGE_GET_POD(device->getStorage(), "ma#nms_threshold", shared_threshold_nms, shared_threshold_nms);
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
    size_t                   current_algorithm_id = 0;
    
    float shared_threshold_score = 0.25;
    float shared_threshold_nms   = 0.45;

    std::atomic<bool> is_ready  = false;
    std::atomic<bool> is_sample = false;
    std::atomic<bool> is_invoke = false;
};

#define static_resource StaticResource::getInstance()

}  // namespace ma::server::callback