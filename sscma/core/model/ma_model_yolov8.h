#ifndef _MA_MODEL_YOLOV8_H_
#define _MA_MODEL_YOLOV8_H_

#include "ma_model_detector.h"

namespace ma::model {

class YoloV8 : public Detector {
   private:
    ma_tensor_t output_;

    int32_t num_record_;
    int32_t num_element_;
    int32_t num_class_;

    enum {
        INDEX_X = 0,
        INDEX_Y = 1,
        INDEX_W = 2,
        INDEX_H = 3,
        INDEX_S = 4,
        INDEX_T = 5,
    };

   protected:
    ma_err_t postprocess() override;

    ma_err_t postProcessI8();
#ifdef MA_MODEL_POSTPROCESS_FP32_VARIANT
    ma_err_t postProcessF32();
#endif

   public:
    YoloV8(Engine* engine);
    ~YoloV8();

    static bool isValid(Engine* engine);

    static const char* getTag();
};

}  // namespace ma::model

#endif  // _MA_MODEL_YOLO_H
