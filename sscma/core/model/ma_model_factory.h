#ifndef _MA_MODEL_FACTORY_H_
#define _MA_MODEL_FACTORY_H_

#include "core/ma_common.h"

#include "ma_model_base.h"

#include "ma_model_classifier.h"
#include "ma_model_detector.h"
#include "ma_model_pose_detector.h"
#include "ma_model_yolov5.h"


namespace ma {

using namespace ma::engine;

class ModelFactory {
public:
    static Model* create(Engine* engine, size_t algorithm_id);
    static ma_err_t remove(Model* model);
};

}  // namespace ma


#endif  // _MA_MODEL_FACTORY_H_