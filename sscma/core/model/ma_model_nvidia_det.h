#ifndef _MA_MODEL_NvidiaDet_H
#define _MA_MODEL_NVIDIA_DET

#include <vector>

#include "ma_model_detector.h"

namespace ma::model {

class _MA_MODEL_NVIDIA_DET : public Detector {
private:
    ma_shape_t conf_shape_;
    ma_shape_t bboxes_shape_;

    int8_t stride_ = 16;
    int8_t scale_  = 35;
    float  offset_ = 0.5;

protected:
    ma_err_t postprocess() override;

    ma_err_t postprocessF32();

public:
    NvidiaDet(Engine* engine);
    ~NvidiaDet();
    static bool isValid(Engine* engine);
};

}  // namespace ma::model

#endif  // _MA_MODEL_YOLO_H
