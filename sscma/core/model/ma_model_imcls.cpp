#include "ma_model_imcls.h"

#include <algorithm>
#include <vector>

namespace ma::model {

constexpr char TAG[] = "ma::model::imcls";

ImCls::ImCls(Engine* p_engine_) : Detector(p_engine_, "imcls", MA_MODEL_TYPE_IMCLS) {
    MA_ASSERT(p_engine_ != nullptr);

    output_ = p_engine_->getOutput(0);

    num_record_  = output_.shape.dims[1];
    num_element_ = output_.shape.dims[2];
    num_class_   = num_element_ - INDEX_T;
}

ImCls::~ImCls() {}

bool ImCls::isValid(Engine* engine) {
    const auto input_shape{engine->getInputShape(0)};
    const auto output_shape{engine->getOutputShape(0)};

    auto is_nhwc{input_shape.dims[3] == 3 || input_shape.dims[3] == 1};

    auto n{0}, h{0}, w{0}, c{0};

    if (input_shape.size != 4) {
        return false;
    }

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

    if (output_shape.size != 2 ||     // B, C
        output_shape.dims[0] != 1 ||  // B = 1
        output_shape.dims[1] < 2      // C >= 2
    ) {
        return false;
    }

    return true;
}

ma_err_t ImCls::postprocess() {
    results_.clear();

    switch (output_.type) {
    case MA_TENSOR_TYPE_S8: {
        const auto* data{output_.data.s8};

        const auto scale{output_.quant_param.scale};
        const bool rescale{scale < 0.1f ? true : false};

        const auto zero_point{output_.quant_param.zero_point};

        const auto output_shape{engine->getOutputShape(0)};
        const auto pred_l{output_shape.dims[1]};

        for (decltype(pred_l) i{0}; i < pred_l; ++i) {
            auto score{static_cast<decltype(scale)>(data[i] - zero_point) * scale};
            score = rescale ? score : score / 100.f;
            if (score > threshold_score_) {
                _results.emplace_back({.score = score, .target = i});
            }
        }

    } break;

    case MA_TENSOR_TYPE_F32: {
        const auto* data{output_.data.f32};

        const auto output_shape{engine->getOutputShape(0)};
        const auto pred_l{output_shape.dims[1]};

        for (decltype(pred_l) i{0}; i < pred_l; ++i) {
            const auto score{data[i]};
            if (score > threshold_score_) {
                _results.emplace_back({.score = score, .target = i});
            }
        }

    } break;

    default:
        MA_LOGE(TAG, "Unsupported tensor type: %d", output_.type);
        return MA_ERROR;
    }

    results_.shrink_to_fit();
    _results.sort([](const auto& a, const auto& b) { return a.score > b.score; });

    return MA_OK;
}

}  // namespace ma::model
