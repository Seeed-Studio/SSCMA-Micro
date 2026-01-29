#ifndef _MA_MODEL_YOLO26_H
#define _MA_MODEL_YOLO26_H

#include <vector>

#include "ma_model_detector.h"

namespace ma::model {

class Yolo26 : public Detector {
private:
    ma_tensor_t outputs_[6];
    int32_t num_record_;
    int32_t num_class_;

protected:
    ma_err_t postprocess() override;

    ma_err_t postProcessI8();
    ma_err_t postProcessF32();


public:
    Yolo26(Engine* engine);
    ~Yolo26();
    static bool isValid(Engine* engine);
};

}  // namespace ma::model

#endif
