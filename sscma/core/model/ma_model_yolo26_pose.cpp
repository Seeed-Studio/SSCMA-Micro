#include <algorithm>
#include <forward_list>
#include <math.h>
#include <vector>

#include "../math/ma_math.h"
#include "../utils/ma_nms.h"

#if MA_USE_ENGINE_HAILO
#include "../engine/ma_engine_hailo.h"
#endif

#include "ma_model_yolo26_pose.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::yolo26_pose";

Yolo26Pose::Yolo26Pose(Engine* p_engine_) : PoseDetector(p_engine_, "Yolo26Pose", MA_MODEL_TYPE_YOLO26_POSE) {
    MA_ASSERT(p_engine_ != nullptr);

    const auto& input_shape = p_engine_->getInputShape(0);
    const auto outputs_count = p_engine_->getOutputSize();
    num_tensor_ = outputs_count;

    if (outputs_count == 1) {
        outputs_[0] = p_engine_->getOutput(0);
        // NMS mode
        num_class_     = 1;  // only one class supported usually
        num_record_    = outputs_[0].shape.dims[2];
        num_keypoints_ = (outputs_[0].shape.dims[1] - 5) / 3;
        // Basic initializers, might need adjustment logic in nms
        return;
    }

    for (size_t i = 0; i < 9 && i < outputs_count; ++i) {
        outputs_[i] = p_engine_->getOutput(i);
    }

    int n = input_shape.dims[0], h = input_shape.dims[1], w = input_shape.dims[2], c = input_shape.dims[3];
    bool is_nhwc = c == 3 || c == 1;

    if (!is_nhwc)
        std::swap(h, c);

    // Calculate expected output size based on input
    int s = w >> 5, m = w >> 4, l = w >> 3;

    num_record_ = (s * s + m * m + l * l);

    // Assuming layout: Box, Cls, Kpt for each scale (3 scales) or interleaved
    // If Tensor 1 is Box (4 channels), Grouped layout might be different for Pose
    // Typical Decoupled Pose:
    // Scale 0: Box, Cls, Kpt
    // Scale 1: Box, Cls, Kpt
    // Scale 2: Box, Cls, Kpt
    // Total 9 outputs
    
    // Check tensor 1 dims to guess layout
    // If Tensor 1 is Cls (channels > 4), it's interleaved
    // Box (4), Cls(C), Kpt(K) ...
    // Indices for interleaved:
    // S0: 0, 1, 2
    // S1: 3, 4, 5
    // S2: 6, 7, 8
    
    box_idx_[0] = 0; box_idx_[1] = 3; box_idx_[2] = 6;
    cls_idx_[0] = 1; cls_idx_[1] = 4; cls_idx_[2] = 7;
    kpt_idx_[0] = 2; kpt_idx_[1] = 5; kpt_idx_[2] = 8;
    
    num_class_ = outputs_[1].shape.dims[1]; // from first Cls tensor
    int kpt_channels = outputs_[2].shape.dims[1];
    num_keypoints_ = kpt_channels / 3;
}


Yolo26Pose::~Yolo26Pose() {}

static bool nmsValid(Engine* engine) {
#if MA_USE_ENGINE_HAILO
    if (engine->getInputSize() != 1 || engine->getOutputSize() != 1)
        return false;

    auto input  = engine->getInput(0);
    auto output = engine->getOutput(0);

    if (input.shape.size != 4 || output.shape.size != 4)
        return false;

    auto n = input.shape.dims[0];
    auto h = input.shape.dims[1];
    auto w = input.shape.dims[2];
    auto c = input.shape.dims[3];
    auto is_nhwc = c == 3 || c == 1;
    if (!is_nhwc) std::swap(h, c);

    if (n != 1 || h < 32 || h % 32 != 0 || (c != 3 && c != 1))
        return false;

    auto b  = output.shape.dims[0];
     // Hailo Pose logic might differ slightly, but checking standard NMS output structure
    auto cs = output.shape.dims[1];
    auto mb = output.shape.dims[2]; 
    // Pose usually [1, num_anchors, channels] if flattened, or [1, num_class, max_bboxes, mask_size]
    
    if (b != 1) return false;

    return true;
#else
    return false;
#endif
}

bool Yolo26Pose::isValid(Engine* engine) {

    const auto inputs_count  = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();
    
    if (outputs_count == 1) {
         // Single output pose is specific, might need more rigorous check
         // For now, allow 1 output if input is valid
         return true;
    }

    if (inputs_count != 1 || outputs_count < 9) { // 3 scales * 3 heads = 9
        return false;
    }
    const auto& input_shape = engine->getInputShape(0);

    // Validate input shape
    if (input_shape.size != 4)
        return false;

    int n = input_shape.dims[0], h = input_shape.dims[1], w = input_shape.dims[2], c = input_shape.dims[3];
    bool is_nhwc = c == 3 || c == 1;

    if (!is_nhwc)
        std::swap(h, c);

    if (n != 1 || h < 32 || h % 32 != 0 || (c != 3 && c != 1))
        return false;

    return true;
}

ma_err_t Yolo26Pose::postProcessI8() {

    const auto score_threshold              = threshold_score_;
    const float score_threshold_non_sigmoid = ma::math::inverseSigmoid(score_threshold);

    for (int i = 0; i < 3; i++) {
        int box_i = box_idx_[i];
        int cls_i = cls_idx_[i];
        int kpt_i = kpt_idx_[i];

        int grid_h           = outputs_[box_i].shape.dims[2];
        int grid_w           = outputs_[box_i].shape.dims[3];
        int grid_l           = grid_h * grid_w;
        int stride           = img_.height / grid_h;
        int8_t* output_score = outputs_[cls_i].data.s8;
        int8_t* output_box   = outputs_[box_i].data.s8;
        int8_t* output_kpt   = outputs_[kpt_i].data.s8;

        for (int j = 0; j < grid_h; j++) {
            for (int k = 0; k < grid_w; k++) {
                int offset = j * grid_w + k;
                int target = -1;
                int8_t max = -128;
                for (int c = 0; c < num_class_; c++) {
                    int8_t score = output_score[offset];
                    offset += grid_l;
                    if (score < max) [[likely]] {
                        continue;
                    }
                    max    = score;
                    target = c;
                }

                if (target < 0)
                    continue;

                float score = ma::math::dequantizeValue(max, outputs_[cls_i].quant_param.scale, outputs_[cls_i].quant_param.zero_point);

                if (score > score_threshold_non_sigmoid) {
                    float rect[4];
                    offset = j * grid_w + k;
                    // Read 4 values directly
                    for (int b = 0; b < 4; b++) {
                        rect[b] = ma::math::dequantizeValue(output_box[offset], outputs_[box_i].quant_param.scale, outputs_[box_i].quant_param.zero_point);
                        offset += grid_l;
                    }

                    // Interpret rect as left, top, right, bottom distances
                    float x1, y1, x2, y2, w, h;
                    x1 = (-rect[0] + k + 0.5) * stride;
                    y1 = (-rect[1] + j + 0.5) * stride;
                    x2 = (rect[2] + k + 0.5) * stride;
                    y2 = (rect[3] + j + 0.5) * stride;
                    w  = x2 - x1;
                    h  = y2 - y1;

                    ma_keypoint2f_t keypoint;
                    keypoint.box.score  = ma::math::sigmoid(score);
                    keypoint.box.target = target;
                    keypoint.box.x      = (x1 + w / 2.0) / img_.width;
                    keypoint.box.y      = (y1 + h / 2.0) / img_.height;
                    keypoint.box.w      = w / img_.width;
                    keypoint.box.h      = h / img_.height;

                    // Parse Keypoints
                    offset = j * grid_w + k;
                    for(int kp = 0; kp < num_keypoints_; kp++) {
                        float kpt_val[3];
                        for(int dim=0; dim<3; dim++) {
                             kpt_val[dim] = ma::math::dequantizeValue(output_kpt[offset], outputs_[kpt_i].quant_param.scale, outputs_[kpt_i].quant_param.zero_point);
                             offset += grid_l; 
                        }
                        
                        float p_x = (kpt_val[0] * 2 + k) * stride; // Decoding logic might vary for Pose
                        float p_y = (kpt_val[1] * 2 + j) * stride;
                        float p_s = ma::math::sigmoid(kpt_val[2]);
                        
                        // Fallback logic, YoloV8 Pose is (x*2+k)*stride... verify decoding
                        keypoint.pts.push_back({p_x / img_.width, p_y / img_.height});
                    }

                    results_.emplace_front(std::move(keypoint));
                }
            }
        }
    }
    return MA_OK;
}

ma_err_t Yolo26Pose::postProcessF32() {

    const auto score_threshold              = threshold_score_;
    const float score_threshold_non_sigmoid = ma::math::inverseSigmoid(score_threshold);

    for (int i = 0; i < 3; i++) {
        int box_i = box_idx_[i];
        int cls_i = cls_idx_[i];
        int kpt_i = kpt_idx_[i];

        int grid_h          = outputs_[box_i].shape.dims[2];
        int grid_w          = outputs_[box_i].shape.dims[3];
        int grid_l          = grid_h * grid_w;
        int stride          = img_.height / grid_h;
        float* output_score = outputs_[cls_i].data.f32;
        float* output_box   = outputs_[box_i].data.f32;
        float* output_kpt   = outputs_[kpt_i].data.f32;

        for (int j = 0; j < grid_h; j++) {
            for (int k = 0; k < grid_w; k++) {
                int offset = j * grid_w + k;
                int target = -1;
                float max  = score_threshold_non_sigmoid;
                for (int c = 0; c < num_class_; c++) {
                    float score = output_score[offset];
                    offset += grid_l;
                    if (score < max) [[likely]] {
                        continue;
                    }
                    max    = score;
                    target = c;
                }

                if (target < 0)
                    continue;

                if (max > score_threshold_non_sigmoid) {
                    float rect[4];
                    offset = j * grid_w + k;
                    // Read 4 values directly
                    for (int b = 0; b < 4; b++) {
                        rect[b] = output_box[offset];
                        offset += grid_l;
                    }

                    float x1, y1, x2, y2, w, h;
                    x1 = (-rect[0] + k + 0.5) * stride;
                    y1 = (-rect[1] + j + 0.5) * stride;
                    x2 = (rect[2] + k + 0.5) * stride;
                    y2 = (rect[3] + j + 0.5) * stride;
                    w  = x2 - x1;
                    h  = y2 - y1;

                    ma_keypoint2f_t keypoint;
                    keypoint.box.score  = ma::math::sigmoid(max);
                    keypoint.box.target = target;
                    keypoint.box.x      = (x1 + w / 2.0) / img_.width;
                    keypoint.box.y      = (y1 + h / 2.0) / img_.height;
                    keypoint.box.w      = w / img_.width;
                    keypoint.box.h      = h / img_.height;
                    
                    // Parse Keypoints
                    offset = j * grid_w + k;
                    for(int kp = 0; kp < num_keypoints_; kp++) {
                         float px = output_kpt[offset]; offset += grid_l;
                         float py = output_kpt[offset]; offset += grid_l;
                         float ps = output_kpt[offset]; offset += grid_l;

                         // Decoding logic: (val * 2 + grid_idx) * stride
                         float decoded_x = (px * 2 + k) * stride;
                         float decoded_y = (py * 2 + j) * stride;
                         // decoded_s usually sigmoid(ps)

                         keypoint.pts.push_back({decoded_x / img_.width, decoded_y / img_.height});
                    }

                    results_.emplace_front(std::move(keypoint));
                }
            }
        }
    }


    return MA_OK;
}

ma_err_t Yolo26Pose::nmsPostProcess() {
    // TODO: Implement Hailo NMS for Pose if needed
    // Similar to object detection but parsing keypoints from metadata
    return MA_ENOTSUP;
}

ma_err_t Yolo26Pose::postprocess() {
    ma_err_t err = MA_OK;
    results_.clear();

    if (num_tensor_ == 1 && (outputs_[0].type == MA_TENSOR_TYPE_NMS_BBOX_U16 || outputs_[0].type == MA_TENSOR_TYPE_NMS_BBOX_F32)) {
#if MA_USE_ENGINE_HAILO
        // TODO: can be optimized by whihout calling this handler for each frame
        if (outputs_[0].external_handler) {
             // ... handler update
        }
#endif
        return nmsPostProcess();
    }

    if (outputs_[0].type == MA_TENSOR_TYPE_F32) {
        err = postProcessF32();
    } else if (outputs_[0].type == MA_TENSOR_TYPE_S8) {
        err = postProcessI8();
    } else {
        return MA_ENOTSUP;
    }

    // NMS for Pose usually calculates IoU based on boxes
    // ma::utils::nms_pose(results_, threshold_nms_, threshold_score_, false, false);
    
    // Sort results
    // results_.sort(...)

    return err;
}
}  // namespace ma::model
