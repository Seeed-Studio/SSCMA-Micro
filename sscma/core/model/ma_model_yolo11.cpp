#include <algorithm>
#include <forward_list>
#include <math.h>
#include <vector>

#include "../math/ma_math.h"
#include "../utils/ma_nms.h"

#include "ma_model_yolo11.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::yolo11";

Yolo11::Yolo11(Engine* p_engine_) : Detector(p_engine_, "Yolo11", MA_MODEL_TYPE_YOLO11) {
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
    num_class_  = outputs_[1].shape.dims[1];
}

Yolo11::~Yolo11() {}

bool Yolo11::isValid(Engine* engine) {

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

    // Calculate expected output size based on input
    int s = w >> 5, m = w >> 4, l = w >> 3;
    int ibox_len = (s * s + m * m + l * l);

    // Validate output shape
    for (size_t i = 0; i < 6; i += 2) {
        auto box_shape = engine->getOutputShape(i);
        auto cls_shape = engine->getOutputShape(i + 1);
        if (box_shape.size != 4 || box_shape.dims[0] != 1 || box_shape.dims[1] != 64 || box_shape.dims[2] != (w >> ((i / 2) + 3)) || box_shape.dims[3] != (w >> ((i / 2) + 3))) {
            return false;
        }
        if (cls_shape.size != 4 || cls_shape.dims[0] != 1 || cls_shape.dims[2] != (w >> ((i / 2) + 3)) || cls_shape.dims[3] != (w >> ((i / 2) + 3))) {
            return false;
        }
    }

    return true;
}

static void compute_dfl(float* tensor, int dfl_len, float* box) {
    for (int b = 0; b < 4; b++) {
        float exp_t[dfl_len];
        float exp_sum = 0;
        float acc_sum = 0;
        for (int i = 0; i < dfl_len; i++) {
            exp_t[i] = exp(tensor[i + b * dfl_len]);
            exp_sum += exp_t[i];
        }

        for (int i = 0; i < dfl_len; i++) {
            acc_sum += exp_t[i] / exp_sum * i;
        }
        box[b] = acc_sum;
    }
}
ma_err_t Yolo11::postProcessI8() {

    int dfl_len                             = outputs_[0].shape.dims[1] / 4;
    const auto score_threshold              = threshold_score_;
    const auto iou_threshold                = threshold_nms_;
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

                float score = ma::math::dequantizeValue(max, outputs_[i * 2].quant_param.scale, outputs_[i * 2].quant_param.zero_point);

                if (score > score_threshold_non_sigmoid) {
                    float rect[4];
                    float before_dfl[dfl_len * 4];
                    offset = j * grid_w + k;
                    for (int b = 0; b < dfl_len * 4; b++) {
                        before_dfl[b] = ma::math::dequantizeValue(output_box[offset], outputs_[i * 2 + 1].quant_param.scale, outputs_[i * 2 + 1].quant_param.zero_point);
                        offset += grid_l;
                    }
                    compute_dfl(before_dfl, dfl_len, rect);

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
ma_err_t Yolo11::postProcessF32() {

    int dfl_len                             = outputs_[0].shape.dims[1] / 4;
    const auto score_threshold              = threshold_score_;
    const auto iou_threshold                = threshold_nms_;
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
                    float before_dfl[dfl_len * 4];
                    offset = j * grid_w + k;
                    for (int b = 0; b < dfl_len * 4; b++) {
                        before_dfl[b] = output_box[offset];
                        offset += grid_l;
                    }
                    compute_dfl(before_dfl, dfl_len, rect);

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

ma_err_t Yolo11::postprocess() {
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
