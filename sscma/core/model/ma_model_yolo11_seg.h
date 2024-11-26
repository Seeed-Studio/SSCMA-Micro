#ifndef _MA_MODEL_YOLO11_SEG_H_
#define _MA_MODEL_YOLO11_SEG_H_

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "ma_model_segmentor.h"

namespace ma::model {

class Yolo11Seg : public Segmentor {
private:
    ma_tensor_t bboxes_;
    ma_tensor_t protos_;
    int32_t num_record_;
    int32_t num_class_;

protected:
    ma_err_t postprocess() override;
    
    ma_err_t postProcessF32();

public:
    Yolo11Seg(Engine* engine);
    ~Yolo11Seg();

    static bool isValid(Engine* engine);
};

}  // namespace ma::model

#endif  // _MA_MODEL_YOLO_H
