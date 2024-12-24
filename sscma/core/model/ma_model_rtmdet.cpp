#include "ma_model_rtmdet.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <numeric>
#include <utility>
#include <vector>

#include "../math/ma_math.h"
#include "../utils/ma_anchors.h"
#include "../utils/ma_nms.h"

namespace ma::model {

static inline decltype(auto) estimateTensorHW(const ma_shape_t& shape) {
    if (shape.size != 4) {
        int32_t ph = 0;
        return std::make_pair(ph, ph);
    }
    const auto is_nhwc{shape.dims[3] == 3 || shape.dims[3] == 1};

    return is_nhwc ? std::make_pair(shape.dims[1], shape.dims[2]) : std::make_pair(shape.dims[2], shape.dims[3]);
}

RTMDet::RTMDet(Engine* p_engine_) : Detector(p_engine_, "rtmdet", MA_MODEL_TYPE_RTMDET) {
    MA_ASSERT(p_engine_ != nullptr);

    for (size_t i = 0; i < num_outputs_; ++i) {
        outputs_[i] = p_engine_->getOutput(i);
    }

    const auto [h, w] = estimateTensorHW(p_engine_->getInputShape(0));

    anchor_strides_ = ma::utils::generateAnchorStrides(std::min(h, w));
    anchor_matrix_  = ma::utils::generateAnchorMatrix(anchor_strides_);


    for (size_t i = 0; i < num_outputs_; ++i) {
        const auto dim_1 = outputs_[i].shape.dims[1];
        const auto dim_2 = outputs_[i].shape.dims[2];

        if (dim_2 == 4) {
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

    using CheckType       = uint8_t;
    size_t    check_bytes = sizeof(CheckType) * 8u;
    CheckType check       = 0;
    for (size_t i = 0; i < anchor_variants_; ++i) {
        CheckType f_s = 1 << (output_scores_ids_[i] % check_bytes);
        CheckType f_b = 1 << (output_bboxes_ids_[i] % check_bytes);
        MA_ASSERT(!(f_s & f_b));
        MA_ASSERT(!(f_s & check));
        MA_ASSERT(!(f_b & check));
        check |= f_s | f_b;
    }
    MA_ASSERT(!(check ^ 0b00111111));
}

RTMDet::~RTMDet() {}

bool RTMDet::isValid(Engine* engine) {
    const auto inputs_count  = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();

    if (inputs_count != 1 || outputs_count != num_outputs_) {
        return false;
    }

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

    auto anchor_strides_1 = ma::utils::generateAnchorStrides(std::min(h, w));
    auto anchor_strides_2 = anchor_strides_1;

    for (size_t i = 0; i < num_outputs_; ++i) {
        const auto output_shape{engine->getOutputShape(i)};
        if (output_shape.size != 3 || output_shape.dims[0] != 1) {
            return false;
        }

        if (output_shape.dims[2] == 4) {
            auto it = std::find_if(
              anchor_strides_2.begin(), anchor_strides_2.end(), [&output_shape](const ma_anchor_stride_t& anchor_stride) {
                  return static_cast<int>(anchor_stride.size) == output_shape.dims[1];
              });
            if (it == anchor_strides_2.end())
                return false;
            else
                anchor_strides_2.erase(it);
        } else {
            auto it = std::find_if(
              anchor_strides_1.begin(), anchor_strides_1.end(), [&output_shape](const ma_anchor_stride_t& anchor_stride) {
                  return static_cast<int>(anchor_stride.size) == output_shape.dims[1];
              });
            if (it == anchor_strides_1.end())
                return false;
            else
                anchor_strides_1.erase(it);
        }
    }

    if (anchor_strides_1.size() || anchor_strides_2.size()) {
        return false;
    }

    return true;
}

const char* RTMDet::getTag() {
    return "ma::model::rmdet";
}

ma_err_t RTMDet::postprocess() {
    uint8_t check = 0;

    for (size_t i = 0; i < num_outputs_; ++i) {
        switch (outputs_[i].type) {
            case MA_TENSOR_TYPE_S8:
                check |= 1 << 8;
                break;

            case MA_TENSOR_TYPE_U8:
                check |= 1 << 7;
                break;

            case MA_TENSOR_TYPE_F32:
                check |= 1 << i;
                break;

            default:
                return MA_ENOTSUP;
        }
    }

    switch (check) {
        case 0b10000000:
            return postProcessI8();

        case 0b01000000:
            return postProcessU8();

#ifdef MA_MODEL_POSTPROCESS_FP32_VARIANT
        case 0b00111111:
            return postProcessF32();
#endif

        default:
            return MA_ENOTSUP;
    }

    return MA_ENOTSUP;
}

ma_err_t RTMDet::postProcessI8() {
    results_.clear();

    const int8_t* output_data[num_outputs_];

    for (size_t i = 0; i < num_outputs_; ++i) {
        output_data[i] = outputs_[i].data.s8;
    }

    const auto score_threshold = threshold_score_;
    const auto iou_threshold   = threshold_nms_;

    const float score_threshold_non_sigmoid = ma::math::inverseSigmoid(score_threshold);

    const auto anchor_matrix_size = anchor_matrix_.size();

    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        const auto output_scores_id             = output_scores_ids_[i];
        const auto* output_scores               = output_data[output_scores_id];
        const size_t output_scores_shape_dims_2 = outputs_[output_scores_id].shape.dims[2];
        const auto output_scores_quant_parm     = outputs_[output_scores_id].quant_param;

        const auto output_bboxes_id             = output_bboxes_ids_[i];
        const auto* output_bboxes               = output_data[output_bboxes_id];
        const size_t output_bboxes_shape_dims_2 = outputs_[output_bboxes_id].shape.dims[2];
        const auto output_bboxes_quant_parm     = outputs_[output_bboxes_id].quant_param;

        const auto& anchor_array     = anchor_matrix_[i];
        const auto anchor_array_size = anchor_array.size();

        const int32_t score_threshold_quan_non_sigmoid = ma::math::quantizeValueFloor(score_threshold_non_sigmoid, output_scores_quant_parm.zero_point, output_scores_quant_parm.scale);

        for (size_t j = 0; j < anchor_array_size; ++j) {
            const auto j_mul_output_scores_shape_dims_2 = j * output_scores_shape_dims_2;

            auto max_score_raw = score_threshold_quan_non_sigmoid;
            int32_t target     = -1;

            for (size_t k = 0; k < output_scores_shape_dims_2; ++k) {
                int8_t score = output_scores[j_mul_output_scores_shape_dims_2 + k];

                if (static_cast<decltype(max_score_raw)>(score) < max_score_raw) [[likely]]
                    continue;

                max_score_raw = score;
                target        = k;
            }

            if (target < 0)
                continue;

            const float real_score = ma::math::sigmoid(ma::math::dequantizeValue(max_score_raw, output_scores_quant_parm.zero_point, output_scores_quant_parm.scale));


            float dist[4];
            const auto pre = j * output_bboxes_shape_dims_2;
            for (size_t m = 0; m < 4; ++m) {
                const size_t offset = pre + m;
                dist[m]  = ma::math::dequantizeValue(static_cast<int32_t>(output_bboxes[offset]), output_bboxes_quant_parm.zero_point, output_bboxes_quant_parm.scale);
            }

            const auto anchor = anchor_array[j];

            float cx = anchor.x + ((dist[2] - dist[0]) * 0.5f);
            float cy = anchor.y + ((dist[3] - dist[1]) * 0.5f);
            float w  = dist[0] + dist[2];
            float h  = dist[1] + dist[3];

            ma_bbox_t res;

            res.x      = cx;
            res.y      = cy;
            res.w      = w;
            res.h      = h;
            res.score  = real_score;
            res.target = target;

            results_.emplace_front(
                std::move(res)
            );
        }
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, true);

    return MA_OK;
}

ma_err_t RTMDet::postProcessU8() {
    results_.clear();

    const uint8_t* output_data[num_outputs_];

    for (size_t i = 0; i < num_outputs_; ++i) {
        output_data[i] = outputs_[i].data.u8;
    }

    const auto score_threshold = threshold_score_;
    const auto iou_threshold   = threshold_nms_;

    const float score_threshold_non_sigmoid = ma::math::inverseSigmoid(score_threshold);

    const auto anchor_matrix_size = anchor_matrix_.size();

    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        const auto output_scores_id             = output_scores_ids_[i];
        const auto* output_scores               = output_data[output_scores_id];
        const size_t output_scores_shape_dims_2 = outputs_[output_scores_id].shape.dims[2];
        const auto output_scores_quant_parm     = outputs_[output_scores_id].quant_param;

        const auto output_bboxes_id             = output_bboxes_ids_[i];
        const auto* output_bboxes               = output_data[output_bboxes_id];
        const size_t output_bboxes_shape_dims_2 = outputs_[output_bboxes_id].shape.dims[2];
        const auto output_bboxes_quant_parm     = outputs_[output_bboxes_id].quant_param;

        const auto& anchor_array     = anchor_matrix_[i];
        const auto anchor_array_size = anchor_array.size();

        const int32_t score_threshold_quan_non_sigmoid = ma::math::quantizeValueFloor(score_threshold_non_sigmoid, output_scores_quant_parm.zero_point, output_scores_quant_parm.scale);

        for (size_t j = 0; j < anchor_array_size; ++j) {
            const auto j_mul_output_scores_shape_dims_2 = j * output_scores_shape_dims_2;

            auto max_score_raw = score_threshold_quan_non_sigmoid;
            int32_t target     = -1;

            for (size_t k = 0; k < output_scores_shape_dims_2; ++k) {
                uint8_t score = output_scores[j_mul_output_scores_shape_dims_2 + k];

                if (static_cast<decltype(max_score_raw)>(score) < max_score_raw) [[likely]]
                    continue;

                max_score_raw = score;
                target        = k;
            }

            if (target < 0)
                continue;

            const float real_score = ma::math::sigmoid(ma::math::dequantizeValue(max_score_raw, output_scores_quant_parm.zero_point, output_scores_quant_parm.scale));

            // DFL
            float dist[4];
            const auto pre = j * output_bboxes_shape_dims_2;
            for (size_t m = 0; m < 4; ++m) {
                const size_t offset = pre + m;
                dist[m]  = ma::math::dequantizeValue(static_cast<int32_t>(output_bboxes[offset]), output_bboxes_quant_parm.zero_point, output_bboxes_quant_parm.scale);
            }

            const auto anchor = anchor_array[j];

            float cx = anchor.x + ((dist[2] - dist[0]) * 0.5f);
            float cy = anchor.y + ((dist[3] - dist[1]) * 0.5f);
            float w  = dist[0] + dist[2];
            float h  = dist[1] + dist[3];

            ma_bbox_t res;

            res.x      = cx;
            res.y      = cy;
            res.w      = w;
            res.h      = h;
            res.score  = real_score;
            res.target = target;

            results_.emplace_front(
                std::move(res)
            );
        }
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, true);

    return MA_OK;
}

#ifdef MA_MODEL_POSTPROCESS_FP32_VARIANT
ma_err_t RTMDet::postProcessF32() {
    results_.clear();

    const float* output_data[num_outputs_];

    for (size_t i = 0; i < num_outputs_; ++i) {
        output_data[i] = outputs_[i].data.f32;
    }

    const auto score_threshold = threshold_score_;
    const auto iou_threshold   = threshold_nms_;

    const float score_threshold_non_sigmoid = ma::math::inverseSigmoid(score_threshold);

    const auto anchor_matrix_size = anchor_matrix_.size();

    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        const auto output_scores_id             = output_scores_ids_[i];
        const auto* output_scores               = output_data[output_scores_id];
        const size_t output_scores_shape_dims_2 = outputs_[output_scores_id].shape.dims[2];

        const auto output_bboxes_id             = output_bboxes_ids_[i];
        const auto* output_bboxes               = output_data[output_bboxes_id];
        const size_t output_bboxes_shape_dims_2 = outputs_[output_bboxes_id].shape.dims[2];

        const auto& anchor_array     = anchor_matrix_[i];
        const auto anchor_array_size = anchor_array.size();

        for (size_t j = 0; j < anchor_array_size; ++j) {
            const auto j_mul_output_scores_shape_dims_2 = j * output_scores_shape_dims_2;

            auto max_score_raw = score_threshold_non_sigmoid;
            int32_t target     = -1;

            for (size_t k = 0; k < output_scores_shape_dims_2; ++k) {
                int8_t score = output_scores[j_mul_output_scores_shape_dims_2 + k];

                if (static_cast<decltype(max_score_raw)>(score) < max_score_raw) [[likely]]
                    continue;

                max_score_raw = score;
                target        = k;
            }

            if (target < 0)
                continue;

            const float real_score = ma::math::sigmoid(max_score_raw);

            float dist[4];
            const auto pre = j * output_bboxes_shape_dims_2;
            for (size_t m = 0; m < 4; ++m) {
                const size_t offset = pre + m;
                dist[m]  = ma::math::dequantizeValue(static_cast<int32_t>(output_bboxes[offset]), output_bboxes_quant_parm.zero_point, output_bboxes_quant_parm.scale);
            }

            const auto anchor = anchor_array[j];

            float cx = anchor.x + ((dist[2] - dist[0]) * 0.5f);
            float cy = anchor.y + ((dist[3] - dist[1]) * 0.5f);
            float w  = dist[0] + dist[2];
            float h  = dist[1] + dist[3];

            ma_bbox_t res;

            res.x      = cx;
            res.y      = cy;
            res.w      = w;
            res.h      = h;
            res.score  = real_score;
            res.target = target;

            results_.emplace_front(
                std::move(res)
            );
        }
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, true);

    return MA_OK;
}
#endif

}  // namespace ma::model
