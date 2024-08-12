#ifndef _MA_MODEL_NVIDIA_DET_
#define _MA_MODEL_NVIDIA_DET_

#include <vector>

#include "ma_model_detector.h"

namespace ma::model {

class NvidiaDet : public Detector {
   private:
    ma_shape_t conf_shape_;
    ma_shape_t bboxes_shape_;

    int8_t stride_ = 16;
    int8_t scale_  = 35;
    float  offset_ = 0.5;

   protected:
    ma_err_t postprocess() override;

    ma_err_t postProcessF32();

   public:
    NvidiaDet(Engine* engine);
    ~NvidiaDet();

    static bool isValid(Engine* engine);

    static const char* getTag();
};

}  // namespace ma::model

#endif
