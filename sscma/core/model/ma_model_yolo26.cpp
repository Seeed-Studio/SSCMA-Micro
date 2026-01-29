#include <algorithm>
#include <forward_list>
#include <math.h>
#include <vector>

#include "../math/ma_math.h"
#include "../utils/ma_nms.h"

#include "ma_model_yolo26.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::yolo26";

Yolo26::Yolo26(Engine* p_engine_) : Detector(p_engine_, "Yolo26", MA_MODEL_TYPE_YOLO26) {
    MA_ASSERT(p_engine_ != nullptr);

    const auto& input_shape = p_engine_->getInputShape(0);

    for (size_t i = 0; i < 6; ++i) {
        outputs_[i] = p_engine_->getOutput(i);
    }

    int n = input_shape.dims[0], h = input_shape.dims[1], w = input_shape.dims[2], c = input_shape.dims[3];
    bool is_nhwc = c == 3 || c == 1;

    if (!is_nhwc)
        std::swap(h, c);

    // Calculate expected output size based on input
    int s = w >> 5, m = w >> 4, l = w >> 3;

    num_record_ = (s * s + m * m + l * l);
    // User stated 3 classes output
    num_class_  = outputs_[1].shape.dims[1];
}

Yolo26::~Yolo26() {}

bool Yolo26::isValid(Engine* engine) {

    const auto inputs_count  = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();

    if (inputs_count != 1 || outputs_count < 6) {
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

    // Validate output shape
    // Assuming 6 outputs: box, cls, box, cls, box, cls
    // User shape example:
    // Tensor 0: shape=[1,4,80,80]
    // Tensor 1: shape=[1,3,80,80]
    // Tensor 2: shape=[1,4,40,40] ...
    for (size_t i = 0; i < 6; i += 2) {
        auto box_shape = engine->getOutputShape(i);
        auto cls_shape = engine->getOutputShape(i + 1);
        int expected_grid = w >> ((i / 2) + 3); // 80, 40, 20 for 640 input
        
        // Box shape: [1, 4, grid, grid]
        if (box_shape.size != 4 || 
            box_shape.dims[0] != 1 || 
            box_shape.dims[1] != 4 ||   // 4 channels for box
            box_shape.dims[2] != expected_grid || 
            box_shape.dims[3] != expected_grid) {
            return false;
        }
        // Cls shape: [1, num_classes, grid, grid]
        if (cls_shape.size != 4 || 
            cls_shape.dims[0] != 1 || 
            cls_shape.dims[2] != expected_grid || 
            cls_shape.dims[3] != expected_grid) {
            return false;
        }
    }

    return true;
}

ma_err_t Yolo26::postProcessI8() {

    const auto score_threshold              = threshold_score_;
    const float score_threshold_non_sigmoid = ma::math::inverseSigmoid(score_threshold);

    for (int i = 0; i < 3; i++) {
        int grid_h           = outputs_[i * 2].shape.dims[2];
        int grid_w           = outputs_[i * 2].shape.dims[3];
        int grid_l           = grid_h * grid_w;
        int stride           = img_.height / grid_h;
        int8_t* output_score = outputs_[i * 2 + 1].data.s8;
        int8_t* output_box   = outputs_[i * 2].data.s8;
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

                float score = ma::math::dequantizeValue(max, outputs_[i * 2 + 1].quant_param.scale, outputs_[i * 2 + 1].quant_param.zero_point);

                if (score > score_threshold_non_sigmoid) {
                    float rect[4];
                    offset = j * grid_w + k;
                    // Read 4 values directly
                    for (int b = 0; b < 4; b++) {
                        rect[b] = ma::math::dequantizeValue(output_box[offset], outputs_[i * 2].quant_param.scale, outputs_[i * 2].quant_param.zero_point);
                        offset += grid_l;
                    }

                    // Interpret rect as left, top, right, bottom distances
                    // Assuming similar to YOLOX/YOLOv11 decoupled head logic without DFL
                    float x1, y1, x2, y2, w, h;
                    x1 = (-rect[0] + k + 0.5) * stride;
                    y1 = (-rect[1] + j + 0.5) * stride;
                    x2 = (rect[2] + k + 0.5) * stride;
                    y2 = (rect[3] + j + 0.5) * stride;
                    w  = x2 - x1;
                    h  = y2 - y1;

                    ma_bbox_t box;
                    box.score  = ma::math::sigmoid(score);
                    box.target = target;
                    box.x      = (x1 + w / 2.0) / img_.width;
                    box.y      = (y1 + h / 2.0) / img_.height;
                    box.w      = w / img_.width;
                    box.h      = h / img_.height;
                    results_.emplace_front(std::move(box));
                }
            }
        }
    }
    return MA_OK;
}

ma_err_t Yolo26::postProcessF32() {

    const auto score_threshold              = threshold_score_;
    const float score_threshold_non_sigmoid = ma::math::inverseSigmoid(score_threshold);

    for (int i = 0; i < 3; i++) {
        int grid_h          = outputs_[i * 2].shape.dims[2];
        int grid_w          = outputs_[i * 2].shape.dims[3];
        int grid_l          = grid_h * grid_w;
        int stride          = img_.height / grid_h;
        float* output_score = outputs_[i * 2 + 1].data.f32;
        float* output_box   = outputs_[i * 2].data.f32;
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

                    ma_bbox_t box;
                    box.score  = ma::math::sigmoid(max);
                    box.target = target;
                    box.x      = (x1 + w / 2.0) / img_.width;
                    box.y      = (y1 + h / 2.0) / img_.height;
                    box.w      = w / img_.width;
                    box.h      = h / img_.height;
                    results_.emplace_front(std::move(box));
                }
            }
        }
    }


    return MA_OK;
}

ma_err_t Yolo26::postprocess() {
    ma_err_t err = MA_OK;
    results_.clear();

    if (outputs_[0].type == MA_TENSOR_TYPE_F32) {
        err = postProcessF32();
    } else if (outputs_[0].type == MA_TENSOR_TYPE_S8) {
        err = postProcessI8();
    } else {
        return MA_ENOTSUP;
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, false);

    results_.sort([](const ma_bbox_t& a, const ma_bbox_t& b) { return a.x < b.x; });

    return err;
}
}  // namespace ma::model
