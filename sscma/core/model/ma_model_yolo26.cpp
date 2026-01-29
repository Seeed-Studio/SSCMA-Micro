#include <algorithm>
#include <forward_list>
#include <math.h>
#include <vector>

#include "../math/ma_math.h"
#include "../utils/ma_nms.h"

#if MA_USE_ENGINE_HAILO
#include "../engine/ma_engine_hailo.h"
#endif

#include "ma_model_yolo26.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::yolo26";

Yolo26::Yolo26(Engine* p_engine_) : Detector(p_engine_, "Yolo26", MA_MODEL_TYPE_YOLO26) {
    MA_ASSERT(p_engine_ != nullptr);

    const auto& input_shape = p_engine_->getInputShape(0);
    const auto outputs_count = p_engine_->getOutputSize();

    if (outputs_count == 1) {
        outputs_[0] = p_engine_->getOutput(0);
        // NMS mode, no further initialization needed here
        return;
    }

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

    // Differentiate between Interleaved (Box, Cls, Box, Cls...) and Grouped (Box, Box, Box, Cls, Cls, Cls) layouts
    if (outputs_[1].shape.dims[1] == 4) {
        // Grouped Layout
        box_idx_[0] = 0; box_idx_[1] = 1; box_idx_[2] = 2;
        cls_idx_[0] = 3; cls_idx_[1] = 4; cls_idx_[2] = 5;
        num_class_  = outputs_[3].shape.dims[1];
    } else {
        // Interleaved Layout
        box_idx_[0] = 0; box_idx_[1] = 2; box_idx_[2] = 4;
        cls_idx_[0] = 1; cls_idx_[1] = 3; cls_idx_[2] = 5;
        num_class_  = outputs_[1].shape.dims[1];
    }
}


Yolo26::~Yolo26() {}

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
    auto cs = output.shape.dims[1];
    auto mb = output.shape.dims[2];
    auto f  = output.shape.dims[3];

    if (b != 1 || cs <= 0 || mb <= 1 || f != 0)
        return false;

    return true;
#else
    return false;
#endif
}

bool Yolo26::isValid(Engine* engine) {

    const auto inputs_count  = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();
    
    if (outputs_count == 1) {
        auto output = engine->getOutput(0);
        if (output.type == MA_TENSOR_TYPE_NMS_BBOX_U16 || output.type == MA_TENSOR_TYPE_NMS_BBOX_F32) {
            return nmsValid(engine);
        }
    }

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

    // Check layouts
    bool is_interleaved = true;
    bool is_grouped = true;

    // Validate Interleaved
    for (size_t i = 0; i < 6; i += 2) {
        auto box_shape = engine->getOutputShape(i);
        auto cls_shape = engine->getOutputShape(i + 1);
        int expected_grid = w >> ((i / 2) + 3); 
        
        if (box_shape.size != 4 || 
            box_shape.dims[1] != 4 ||
            box_shape.dims[2] != expected_grid || 
            box_shape.dims[3] != expected_grid) {
            is_interleaved = false;
        }
        if (cls_shape.size != 4 || 
            cls_shape.dims[2] != expected_grid || 
            cls_shape.dims[3] != expected_grid) {
            is_interleaved = false;
        }
    }

    if (is_interleaved) return true;

    // Validate Grouped
    for (size_t i = 0; i < 3; ++i) {
        auto box_shape = engine->getOutputShape(i);
        auto cls_shape = engine->getOutputShape(i + 3);
        int expected_grid = w >> (i + 3); 

        if (box_shape.size != 4 || 
            box_shape.dims[1] != 4 ||
            box_shape.dims[2] != expected_grid || 
            box_shape.dims[3] != expected_grid) {
            is_grouped = false;
        }
        if (cls_shape.size != 4 || 
            cls_shape.dims[2] != expected_grid || 
            cls_shape.dims[3] != expected_grid) {
            is_grouped = false;
        }
    }

    return is_grouped;
}

ma_err_t Yolo26::postProcessI8() {

    const auto score_threshold              = threshold_score_;
    const float score_threshold_non_sigmoid = ma::math::inverseSigmoid(score_threshold);

    for (int i = 0; i < 3; i++) {
        int box_i = box_idx_[i];
        int cls_i = cls_idx_[i];

        int grid_h           = outputs_[box_i].shape.dims[2];
        int grid_w           = outputs_[box_i].shape.dims[3];
        int grid_l           = grid_h * grid_w;
        int stride           = img_.height / grid_h;
        int8_t* output_score = outputs_[cls_i].data.s8;
        int8_t* output_box   = outputs_[box_i].data.s8;

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
        int box_i = box_idx_[i];
        int cls_i = cls_idx_[i];

        int grid_h          = outputs_[box_i].shape.dims[2];
        int grid_w          = outputs_[box_i].shape.dims[3];
        int grid_l          = grid_h * grid_w;
        int stride          = img_.height / grid_h;
        float* output_score = outputs_[cls_i].data.f32;
        float* output_box   = outputs_[box_i].data.f32;

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

ma_err_t Yolo26::nmsPostProcess() {
#if MA_USE_ENGINE_HAILO

    auto& output = outputs_[0];

    if (output.shape.size < 4) {
        return MA_FAILED;
    }

    size_t w = output.shape.dims[1];
    size_t h = output.shape.dims[2];
    size_t c = output.shape.dims[3];

    hailo_nms_shape_t nms_shape;
    if (output.external_handler) {
        auto rc = (*reinterpret_cast<ma::engine::EngineHailo::ExternalHandler*>(output.external_handler))(4, &nms_shape, sizeof(hailo_nms_shape_t));
        if (rc == MA_OK) {
            w = nms_shape.number_of_classes;
            h = nms_shape.max_bboxes_per_class;
            c = nms_shape.max_accumulated_mask_size;
        }
    }

    switch (output.type) {
        case MA_TENSOR_TYPE_NMS_BBOX_U16: {
            using T = uint16_t;
            using P = hailo_bbox_t;

            const auto zp    = output.quant_param.zero_point;
            const auto scale = output.quant_param.scale;

            auto ptr = output.data.u8;
            for (size_t i = 0; i < w; ++i) {
                auto bc = *reinterpret_cast<T*>(ptr);
                ptr += sizeof(T);

                if (bc <= 0) {
                    continue;
                } else if (bc > h) {
                    break;
                }

                for (size_t j = 0; j < static_cast<size_t>(bc); ++j) {
                    auto bbox = *reinterpret_cast<P*>(ptr);
                    ptr += sizeof(P);

                    ma_bbox_t res;

                    auto x_min = static_cast<float>(bbox.x_min - zp) * scale;
                    auto y_min = static_cast<float>(bbox.y_min - zp) * scale;
                    auto x_max = static_cast<float>(bbox.x_max - zp) * scale;
                    auto y_max = static_cast<float>(bbox.y_max - zp) * scale;
                    res.w      = x_max - x_min;
                    res.h      = y_max - y_min;
                    res.x      = x_min + res.w * 0.5;
                    res.y      = y_min + res.h * 0.5;
                    res.score  = static_cast<float>(bbox.score - zp) * scale;

                    res.target = static_cast<int>(i);

                    res.x = MA_CLIP(res.x, 0, 1.0f);
                    res.y = MA_CLIP(res.y, 0, 1.0f);
                    res.w = MA_CLIP(res.w, 0, 1.0f);
                    res.h = MA_CLIP(res.h, 0, 1.0f);

                    results_.emplace_front(res);
                }
            }
        } break;

        case MA_TENSOR_TYPE_NMS_BBOX_F32: {
            using T = float32_t;
            using P = hailo_bbox_float32_t;

            auto ptr = output.data.u8;
            for (size_t i = 0; i < w; ++i) {
                auto bc = *reinterpret_cast<T*>(ptr);
                ptr += sizeof(T);

                if (bc <= 0) {
                    continue;
                } else if (bc > h) {
                    break;
                }

                for (size_t j = 0; j < static_cast<size_t>(bc); ++j) {
                    auto bbox = *reinterpret_cast<P*>(ptr);
                    ptr += sizeof(P);

                    ma_bbox_t res;

                    res.w     = bbox.x_max - bbox.x_min;
                    res.h     = bbox.y_max - bbox.y_min;
                    res.x     = bbox.x_min + res.w * 0.5;
                    res.y     = bbox.y_min + res.h * 0.5;
                    res.score = bbox.score;

                    res.target = static_cast<int>(i);

                    res.x = MA_CLIP(res.x, 0, 1.0f);
                    res.y = MA_CLIP(res.y, 0, 1.0f);
                    res.w = MA_CLIP(res.w, 0, 1.0f);
                    res.h = MA_CLIP(res.h, 0, 1.0f);

                    results_.emplace_front(res);
                }
            }
        } break;

        default:
            return MA_ENOTSUP;
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, false);

    results_.sort([](const ma_bbox_t& a, const ma_bbox_t& b) { return a.x < b.x; });

    return MA_OK;
#else
    return MA_FAILED;
#endif
}

ma_err_t Yolo26::postprocess() {
    ma_err_t err = MA_OK;
    results_.clear();

    if (outputs_[0].type == MA_TENSOR_TYPE_NMS_BBOX_U16 || outputs_[0].type == MA_TENSOR_TYPE_NMS_BBOX_F32) {
#if MA_USE_ENGINE_HAILO
        // TODO: can be optimized by whihout calling this handler for each frame
        if (outputs_[0].external_handler) {
            auto ph   = reinterpret_cast<ma::engine::EngineHailo::ExternalHandler*>(outputs_[0].external_handler);
            float thr = threshold_score_;
            auto rc   = (*ph)(1, &thr, sizeof(float));
            if (rc == MA_OK) {
                threshold_score_ = thr;
            }
            thr = threshold_nms_;
            rc  = (*ph)(3, &thr, sizeof(float));
            if (rc == MA_OK) {
                threshold_nms_ = thr;
            }
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

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, false);

    results_.sort([](const ma_bbox_t& a, const ma_bbox_t& b) { return a.x < b.x; });

    return err;
}
}  // namespace ma::model
