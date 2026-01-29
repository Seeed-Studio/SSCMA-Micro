#ifndef _MA_MODEL_YOLO26_SEG_H
#define _MA_MODEL_YOLO26_SEG_H

#include <vector>

#include "ma_model_segmentor.h"

namespace ma::model {

class Yolo26Seg : public Segmentor {
private:
    ma_tensor_t outputs_[6];      // Det outputs: Box, Cls, MaskCoef?
    ma_tensor_t proto_;           // Protonet output
    int32_t num_record_;
    int32_t num_class_;
    int32_t num_mask_cmds_;
    
    // Indices mapping
    int8_t box_idx_[3];
    int8_t cls_idx_[3];
    int8_t msk_idx_[3];

protected:
    ma_err_t postprocess() override;

    ma_err_t postProcessF32();
    // ma_err_t postProcessI8(); // Usually Segmentation models are F32 or mixed, keeping simpler first

public:
    Yolo26Seg(Engine* engine);
    ~Yolo26Seg();
    static bool isValid(Engine* engine);
};

}  // namespace ma::model

#endif
