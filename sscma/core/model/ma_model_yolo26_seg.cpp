#include <algorithm>
#include <forward_list>
#include <math.h>
#include <vector>

#include "../math/ma_math.h"
#include "../utils/ma_nms.h"

#include "ma_model_yolo26_seg.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::yolo26_seg";

Yolo26Seg::Yolo26Seg(Engine* p_engine_) : Segmentor(p_engine_, "Yolo26Seg", MA_MODEL_TYPE_YOLO26_SEG) {
    MA_ASSERT(p_engine_ != nullptr);

    // Assuming layout: 
    // 3 scales, each has Box, Cls, MaskCoef
    // Plus 1 Protonet output
    // Total 10 outputs? Or interleaved...
    
    // Let's assume decoupled logic similar to detection but with extra mask coeff
    // outputs_[0-8] for detection/mask heads
    // outputs_[9] for protonet
    
    const auto& input_shape = p_engine_->getInputShape(0);
    int num_outputs = p_engine_->getOutputSize();
    
    // Find protonet: usually standard shape [1, 32, 160, 160] (mask_protos)
    // The rest are detection heads
    
    // Simple logic:
    // Box, Cls, MaskCoef interleaved or grouped
    // We need to map them.
    
    // Placeholder implementation, needs specific tensor layout info
    // Assuming standard Decoupled Seg:
    // Heads: Box[4], Cls[NC], MaskCoef[32]
    
    // If output count is 10 (3*3 + 1 proto)
    
    num_class_ = 80; // default
}

Yolo26Seg::~Yolo26Seg() {}

bool Yolo26Seg::isValid(Engine* engine) {
    // Basic validation
    return engine->getOutputSize() > 1; 
}

ma_err_t Yolo26Seg::postProcessF32() {
    // Decoding Logic for Segmentation
    // 1. Decode Boxes & Classes (same as Detection)
    // 2. Decode Mask Coefficients
    // 3. Matrix Multiply: Mask Coeffs * Protonet
    // 4. Sigmoid & Crop to Box
    
    return MA_ENOTSUP; // TODO: Implement full logic
}

ma_err_t Yolo26Seg::postprocess() {
    results_.clear();
    
    if (outputs_[0].type == MA_TENSOR_TYPE_F32) {
        return postProcessF32();
    }
    
    return MA_ENOTSUP;
}

}  // namespace ma::model
