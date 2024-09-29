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
    case 1:
        if (FOMO::isValid(engine)) {
            return new FOMO(engine);
        }

    case 2:
        if (PFLD::isValid(engine)) {
            return new PFLD(engine);
        }

    case 3:
        if (YoloV5::isValid(engine)) {
            return new YoloV5(engine);
        }

    case 4:
        if (Classifier::isValid(engine)) {
            return new Classifier(engine);
        }

    case 5:
        if (YoloV8Pose::isValid(engine)) {
            return new YoloV8Pose(engine);
        }

    case 6:
        if (YoloV8::isValid(engine)) {
            return new YoloV8(engine);
        }

    case 7:
        if (NvidiaDet::isValid(engine)) {
            return new NvidiaDet(engine);
        }

    case 8:
        if (YoloWorld::isValid(engine)) {
            return new YoloWorld(engine);
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