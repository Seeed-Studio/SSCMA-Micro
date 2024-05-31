#ifndef _MA_MODEL_YOLO_H
#define _MA_MODEL_YOLO_H

#include <vector>

#include "core/model/ma_model_base.h"
#include "core/model/ma_model_detector.h"


namespace ma::model {
class Yolo : public Detector {
private:
    ma_tensor_t output_;
    int32_t     num_record_;
    int32_t     num_element_;
    int32_t     num_class_;
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

public:
    Yolo(Engine* engine);
    ~Yolo();
    bool is_valid(Engine* engine) override;
};

}  // namespace ma::model


#endif  // _MA_MODEL_YOLO_H