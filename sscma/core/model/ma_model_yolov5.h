#ifndef _MA_MODEL_YOLOV5_H
#define _MA_MODEL_YOLOV5_H

#include <vector>

#include "ma_model_detector.h"

namespace ma::model {

class YoloV5 : public Detector {
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
    ma_err_t generalPostProcess();
    ma_err_t nmsPostProcess();

public:
    YoloV5(Engine* engine);
    ~YoloV5();
    static bool isValid(Engine* engine);
};

}  // namespace ma::model

#endif  // _MA_MODEL_YOLO_H
