#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <utility>
#include <vector>

#include "core/utils/ma_nms.h"
#include "ma_model_world.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::yolo_world";

static inline decltype(auto) estimateTensorHW(const ma_shape_t& shape) {
    if (shape.size != 4) {
        return std::make_pair(0, 0);
    }
    const auto is_nhwc{shape.dims[3] == 3 || shape.dims[3] == 1};

    return is_nhwc ? std::make_pair(shape.dims[1], shape.dims[2]) : std::make_pair(shape.dims[2], shape.dims[3]);
}

static inline float fastLn(float x) {
    // Remez approximation of ln(x)
    static_assert(sizeof(unsigned int) == sizeof(float));
    static_assert(sizeof(float) == 4);

    auto bx{*reinterpret_cast<unsigned int*>(&x)};
    auto ex{bx >> 23};
    auto t{static_cast<signed int>(ex) - static_cast<signed int>(127)};
    bx = 1065353216 | (bx & 8388607);
    x  = *reinterpret_cast<float*>(&bx);
    return -1.49278 + (2.11263 + (-0.729104 + 0.10969 * x) * x) * x + 0.6931471806 * t;
}

static inline float fastExp(float x) {
    // N. Schraudolph, "A Fast, Compact Approximation of the Exponential Function"
    // https://nic.schraudolph.org/pubs/Schraudolph99.pdf
    static_assert(sizeof(float) == 4);

    x = (12102203.1608621 * x) + 1064986823.01029;

    const float c{8388608.f};
    const float d{2139095040.f};

    if (x < c || x > d) x = (x < c) ? 0.0f : d;

    uint32_t n = static_cast<uint32_t>(x);
    x          = *reinterpret_cast<float*>(&n);

    return x;
}

static inline float sigmoid(float x) { return 1.f / (1.f + fastExp(-x)); }

static inline void softmax(float* data, size_t size) {
    float sum{0.f};
    for (size_t i = 0; i < size; ++i) {
        data[i] = std::exp(data[i]);
        sum += data[i];
    }
    for (size_t i = 0; i < size; ++i) {
        data[i] /= sum;
    }
}

YoloWorld::YoloWorld(Engine* p_engine_) : Detector(p_engine_, "yolo_world", MA_MODEL_TYPE_YOLO_WORLD) {
    MA_ASSERT(p_engine_ != nullptr);

    for (size_t i = 0; i < outputs_; ++i) {
        output_tensors_[i]      = p_engine_->getOutput(i);
        output_shapes_[i]       = p_engine->getOutputShape(i);
        output_quant_params_[i] = p_engine->getOutputQuantParam(i);
    }

    const auto [h, w] = estimateTensorHW(p_engine_->getInputShape(0));
    MA_ASSERT(h & w);

    anchor_strides_ = generateAnchorStrides_(std::min(h, w));
    anchor_matrix_  = generateAnchorMatrix_(anchor_strides_);

    for (size_t i = 0; i < outputs_; ++i) {
        const auto dim_1 = output_shapes_[i].dims[1];
        const auto dim_2 = output_shapes_[i].dims[2];

        if (dim_2 == 64) {
            for (size_t j = 0; j < anchor_variants_; ++j) {
                if (dim_1 == static_cast<int>(anchor_strides_[j].size)) {
                    output_bboxes_ids_[j] = i;
                    break;
                }
            }
        } else {
            for (size_t j = 0; j < anchor_variants_; ++j) {
                if (dim_1 == static_cast<int>(anchor_strides_[j].size)) {
                    output_scores_ids_[j] = i;
                    break;
                }
            }
        }
    }

    using CheckType          = uint8_t;
    const size_t check_bytes = sizeof(CheckType) * 8u;
    CheckType    check       = 0;
    for (size_t i = 0; i < anchor_variants_; ++i) {
        CheckType f_s = 1 << (output_scores_ids_[i] % check_bytes);
        CheckType f_b = 1 << (output_bboxes_ids_[i] % check_bytes);
        check |= f_s | f_b;
    }
    MA_ASSERT(!(check ^ 0b00111111));
}

YoloWorld::~YoloWorld() {}

bool YoloWorld::isValid(Engine* engine) {
    const auto input_shape{engine->getInputShape(0)};

    if (input_shape.size != 4) {
        return false;
    }

    const auto is_nhwc{input_shape.dims[3] == 3 || input_shape.dims[3] == 1};

    size_t n = 0, h = 0, w = 0, c = 0;
    if (is_nhwc) {
        n = input_shape.dims[0];
        h = input_shape.dims[1];
        w = input_shape.dims[2];
        c = input_shape.dims[3];
    } else {
        n = input_shape.dims[0];
        c = input_shape.dims[1];
        h = input_shape.dims[2];
        w = input_shape.dims[3];
    }

    if (n != 1 || h ^ w || h < 32 || h % 32 || (c != 3 && c != 1)) {
        return false;
    }

    auto anchor_strides_1 = generateAnchorStrides_(std::min(h, w));
    auto anchor_strides_2 = anchor_strides_1;

    // Note: would fail if the model has 64 classes
    for (size_t i = 0; i < outputs_; ++i) {
        const auto output_shape{engine->getOutputShape(i)};
        if (output_shape.size != 3 || output_shape.dims[0] != 1) return false;

        if (output_shape.dims[2] == 64) {
            auto it = std::find_if(
              anchor_strides_2.begin(), anchor_strides_2.end(), [&output_shape](const anchor_stride_t& anchor_stride) {
                  return static_cast<int>(anchor_stride.size) == output_shape.dims[1];
              });
            if (it == anchor_strides_2.end())
                return false;
            else
                anchor_strides_2.erase(it);
        } else {
            auto it = std::find_if(
              anchor_strides_1.begin(), anchor_strides_1.end(), [&output_shape](const anchor_stride_t& anchor_stride) {
                  return static_cast<int>(anchor_stride.size) == output_shape.dims[1];
              });
            if (it == anchor_strides_1.end())
                return false;
            else
                anchor_strides_1.erase(it);
        }
    }

    if (anchor_strides_1.size() || anchor_strides_2.size()) return false;

    return true;
}

ma_err_t YoloWorld::postprocess() {
    uint8_t check = 0;

    for (size_t i = 0; i < outputs_; ++i) {
        switch (output_tensors_.type) {
        case MA_TENSOR_TYPE_S8:
            break;
        case MA_TENSOR_TYPE_F32:
            check |= 1 << i;
            break;
        default:
            return MA_ENOTSUP;
        }
    }

    if (check != 0 && (check ^ 0b111111)) return MA_ENOTSUP;

    return MA_OK;
}

ma_err_t YoloWorld::postprocessI8() {
    results_.clear();

    const int8_t* output_data[_outputs];
    // Note: assume is dynamic tensor
    for (size_t i = 0; i < outputs_; ++i) {
        output_tensors_[i] = p_engine_->getOutput(i);
        output_data[i]     = output_tensors_[i].data.s8;
    }

    const uint8_t score_threshold = this->threshold_score_;
    const uint8_t iou_threshold   = this->threshold_nms_;

    const float score_threshold_non_sigmoid = -std::log((1.0 / static_cast<float>(score_threshold)) - 1.f);

    const auto anchor_matrix_size = anchor_matrix_.size();

    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        const auto   output_scores_id           = output_scores_ids_[i];
        const auto*  output_scores              = output_data[output_scores_id];
        const size_t output_scores_shape_dims_2 = output_shapes_[output_scores_id].dims[2];
        const auto   output_scores_quant_parm   = output_quant_params_[output_scores_id];

        const auto   output_bboxes_id           = output_bboxes_ids_[i];
        const auto*  output_bboxes              = output_data[output_bboxes_id];
        const size_t output_bboxes_shape_dims_2 = output_shapes_[output_bboxes_id].dims[2];
        const auto   output_bboxes_quant_parm   = output_quant_params_[output_bboxes_id];

        const auto& anchor_array      = anchor_matrix_[i];
        const auto  anchor_array_size = anchor_array.size();

        const int32_t score_threshold_quan_non_sigmoid =
          static_cast<int32_t>(std::floor(score_threshold_non_sigmoid / output_scores_quant_parm.scale)) +
          output_scores_quant_parm.zero_point;

        for (size_t j = 0; j < anchor_array_size; ++j) {
            const auto j_mul_output_scores_shape_dims_2 = j * output_scores_shape_dims_2;

            auto    max_score_raw = score_threshold_quan_non_sigmoid;
            int32_t target        = -1;

            for (size_t k = 0; k < output_scores_shape_dims_2; ++k) {
                int8_t score = output_scores[j_mul_output_scores_shape_dims_2 + k];

                if (static_cast<decltype(max_score_raw)>(score) < max_score_raw) [[likely]]
                    continue;

                max_score_raw = score;
                target        = k;
            }

            if (target < 0) continue;

            const float real_score = sigmoid(static_cast<float>(max_score_raw - output_scores_quant_parm.zero_point) *
                                             output_scores_quant_parm.scale);

            // DFL
            float dist[4];
            float matrix[16];

            const auto pre = j * output_bboxes_shape_dims_2;
            for (size_t m = 0; m < 4; ++m) {
                const size_t offset = pre + m * 16;
                for (size_t n = 0; n < 16; ++n) {
                    matrix[n] = dequantValue_(
                      offset + n, output_bboxes, output_bboxes_quant_parm.zero_point, output_bboxes_quant_parm.scale);
                }

                softmax(matrix, 16);

                float res = 0.0;
                for (size_t n = 0; n < 16; ++n) {
                    res += matrix[n] * static_cast<float>(n);
                }
                dist[m] = res;
            }

            const auto anchor = anchor_array[j];

            float cx = anchor.x + ((dist[2] - dist[0]) * 0.5f);
            float cy = anchor.y + ((dist[3] - dist[1]) * 0.5f);
            float w  = dist[0] + dist[2];
            float h  = dist[1] + dist[3];

            results_.emplace_back(ma_bbox_t{.x = cx, .y = cy, .w = w, .h = h, .score = real_score, .target = target});
        }
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, false);

    results_.shrink_to_fit();

    return MA_OK;
}

ma_err_t YoloWorld::postprocessF32() {
    results_.clear();

    const float* output_data[_outputs];
    // Note: assume is dynamic tensor
    for (size_t i = 0; i < outputs_; ++i) {
        output_tensors_[i] = p_engine_->getOutput(i);
        output_data[i]     = output_tensors_[i].data.f32;
    }

    const uint8_t score_threshold = this->threshold_score_;
    const uint8_t iou_threshold   = this->threshold_nms_;

    const float score_threshold_non_sigmoid = -std::log((1.0 / static_cast<float>(score_threshold)) - 1.f);

    const auto anchor_matrix_size = anchor_matrix_.size();

    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        const auto   output_scores_id           = output_scores_ids_[i];
        const auto*  output_scores              = output_data[output_scores_id];
        const size_t output_scores_shape_dims_2 = output_shapes_[output_scores_id].dims[2];

        const auto   output_bboxes_id           = output_bboxes_ids_[i];
        const auto*  output_bboxes              = output_data[output_bboxes_id];
        const size_t output_bboxes_shape_dims_2 = output_shapes_[output_bboxes_id].dims[2];

        const auto& anchor_array      = anchor_matrix_[i];
        const auto  anchor_array_size = anchor_array.size();

        for (size_t j = 0; j < anchor_array_size; ++j) {
            const auto j_mul_output_scores_shape_dims_2 = j * output_scores_shape_dims_2;

            auto    max_score_raw = score_threshold_non_sigmoid;
            int32_t target        = -1;

            for (size_t k = 0; k < output_scores_shape_dims_2; ++k) {
                int8_t score = output_scores[j_mul_output_scores_shape_dims_2 + k];

                if (static_cast<decltype(max_score_raw)>(score) < max_score_raw) [[likely]]
                    continue;

                max_score_raw = score;
                target        = k;
            }

            if (target < 0) continue;

            const float real_score = sigmoid(max_score_raw);

            // DFL
            float dist[4];
            float matrix[16];

            const auto pre = j * output_bboxes_shape_dims_2;
            for (size_t m = 0; m < 4; ++m) {
                const size_t offset = pre + m * 16;
                for (size_t n = 0; n < 16; ++n) {
                    matrix[n] = output_bboxes_id[offset + n];
                }

                softmax(matrix, 16);

                float res = 0.0;
                for (size_t n = 0; n < 16; ++n) {
                    res += matrix[n] * static_cast<float>(n);
                }
                dist[m] = res;
            }

            const auto anchor = anchor_array[j];

            float cx = anchor.x + ((dist[2] - dist[0]) * 0.5f);
            float cy = anchor.y + ((dist[3] - dist[1]) * 0.5f);
            float w  = dist[0] + dist[2];
            float h  = dist[1] + dist[3];

            results_.emplace_back(ma_bbox_t{.x = cx, .y = cy, .w = w, .h = h, .score = real_score, .target = target});
        }
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, false);

    results_.shrink_to_fit();

    return MA_OK;
}

}  // namespace ma::model
