#ifndef _MA_MODEL_RTMDET_H_
#define _MA_MODEL_RTMDET_H_

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "ma_model_detector.h"

namespace ma::model {

class RTMDet : public Detector {
   private:
    static constexpr size_t num_outputs_     = 6;
    static constexpr size_t anchor_variants_ = 3;

    ma_tensor_t outputs_[num_outputs_];

    std::vector<ma_anchor_stride_t>     anchor_strides_;
    std::vector<std::vector<ma_pt2f_t>> anchor_matrix_;

    size_t output_scores_ids_[anchor_variants_];
    size_t output_bboxes_ids_[anchor_variants_];

   protected:
    ma_err_t postprocess() override;

    ma_err_t postProcessI8();
    ma_err_t postProcessU8();
#ifdef MA_MODEL_POSTPROCESS_FP32_VARIANT
    ma_err_t postProcessF32();
#endif

   public:
    RTMDet(Engine* engine);
    ~RTMDet();

    static bool isValid(Engine* engine);

    static const char* getTag();
};

}  // namespace ma::model

#endif  // _MA_MODEL_YOLO_H
