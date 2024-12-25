#include "ma_model_factory.h"

namespace ma {
constexpr char TAG[] = "ma::model::factory";

using namespace ma::model;

Model* ModelFactory::create(Engine* engine, size_t algorithm_id) {
    if (engine == nullptr) {
        return nullptr;
    }

    switch (algorithm_id) {
        case 0:
        case MA_MODEL_TYPE_FOMO:
            if (FOMO::isValid(engine)) {
                return new FOMO(engine);
            }

        case MA_MODEL_TYPE_IMCLS:
            if (Classifier::isValid(engine)) {
                return new Classifier(engine);
            }

        case MA_MODEL_TYPE_PFLD:
            if (PFLD::isValid(engine)) {
                return new PFLD(engine);
            }

        case MA_MODEL_TYPE_YOLOV5:
            if (YoloV5::isValid(engine)) {
                return new YoloV5(engine);
            }

        case MA_MODEL_TYPE_YOLOV8_POSE:
#if MA_USE_ENGINE_HAILO
            if (YoloV8PoseHailo::isValid(engine)) {
                return new YoloV8PoseHailo(engine);
            }

            if (YoloV8SegHailo::isValid(engine)) {
                return new YoloV8SegHailo(engine);
            }
#endif
            if (YoloV8Pose::isValid(engine)) {
                return new YoloV8Pose(engine);
            }

        case MA_MODEL_TYPE_YOLOV8:
            if (YoloV8::isValid(engine)) {
                return new YoloV8(engine);
            }

        case MA_MODEL_TYPE_NVIDIA_DET:
            if (NvidiaDet::isValid(engine)) {
                return new NvidiaDet(engine);
            }

        case MA_MODEL_TYPE_RTMDET:
            if (RTMDet::isValid(engine)) {
                return new RTMDet(engine);
            }
        
        case MA_MODEL_TYPE_YOLO_WORLD:
            if (YoloWorld::isValid(engine)) {
                return new YoloWorld(engine);
            }
        case MA_MODEL_TYPE_YOLO11:
            if (Yolo11::isValid(engine)) {
                return new Yolo11(engine);
            }
        case MA_MODEL_TYPE_YOLO11_POSE:
            if (Yolo11Pose::isValid(engine)) {
                return new Yolo11Pose(engine);
            }
        case MA_MODEL_TYPE_YOLO11_SEG:
            if (Yolo11Seg::isValid(engine)) {
                return new Yolo11Seg(engine);
            }
    }

    return nullptr;
}

ma_err_t ModelFactory::remove(Model* model) {
    if (model == nullptr) {
        return MA_EINVAL;
    }
    delete model;
    return MA_OK;
}

}  // namespace ma