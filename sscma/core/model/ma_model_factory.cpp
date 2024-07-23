#include "ma_model_factory.h"


namespace ma::model {
constexpr char TAG[] = "ma::model::factory";

Model* ModelFactory::create(Engine* engine) {

    if (engine == nullptr) {
        return nullptr;
    }

    if (YoloV5::isValid(engine)) {
        MA_LOGI(TAG, "create yolo v5 model");
        return new YoloV5(engine);
    }
    if (Classifier::isValid(engine)) {
        MA_LOGI(TAG, "create classifier model");
        return new Classifier(engine);
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

}  // namespace ma::model