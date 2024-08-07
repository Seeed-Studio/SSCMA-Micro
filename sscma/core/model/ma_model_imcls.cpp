#include "ma_model_imcls.h"

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/math/ma_math.h"

namespace ma::model {

ImCls::ImCls(Engine* p_engine_) : Detector(p_engine_, "imcls", MA_MODEL_TYPE_IMCLS) {
    MA_ASSERT(p_engine_ != nullptr);

    output_ = p_engine_->getOutput(0);
}

ImCls::~ImCls() {}

bool ImCls::isValid(Engine* engine) {
    const auto input_shape{engine->getInputShape(0)};
    const auto output_shape{engine->getOutputShape(0)};

    if (input_shape.size != 4) {
        return false;
    }

    auto is_nhwc{input_shape.dims[3] == 3 || input_shape.dims[3] == 1};

    size_t n = 0, h = 0, w = 0, c = 0;

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

const char* ImCls::getTag() { return "ma::model::imcls"; }

ma_err_t ImCls::postprocess() {
    switch (output_.type) {
    case MA_TENSOR_TYPE_S8:
        return postProcessI8();

    case MA_TENSOR_TYPE_F32:
        return postProcessF32();

    default:
        return MA_ENOTSUP;
    }

    return MA_ENOTSUP;
}

ma_err_t ImCls::postProcessI8() {
    results_.clear();

    const auto* data{output_.data.s8};

    const auto scale{output_.quant_param.scale};
    const auto zero_point{output_.quant_param.zero_point};
    const bool normalized{
      (scale * (std::numeric_limits<decltype(*data)>::max() - std::numeric_limits<decltype(*data)>::min())) <= 1.0};

    const auto pred_l{output_.shape.dims[1]};

    for (decltype(pred_l) i = 0; i < pred_l; ++i) {
        float score = ma::math::dequantizeValue(static_cast<int32_t>(data[i]), scale, zero_point);
        score       = normalized ? score : score / 100.f;

        if (score >= threshold_score_) {
            results_.emplace_back({.score = score, .target = i});
        }
    }

    results_.shrink_to_fit();

    results_.sort([](const auto& a, const auto& b) { return a.score > b.score; });

    return MA_OK;
}

ma_err_t ImCls::postProcessF32() {
    results_.clear();

    const auto* data{output_.data.f32};
    const auto  pred_l{output_.shape.dims[1]};

    for (decltype(pred_l) i = 0; i < pred_l; ++i) {
        const float score = data[i];

        if (score >= threshold_score_) {
            results_.emplace_back({.score = score, .target = i});
        }
    }

    results_.shrink_to_fit();

    results_.sort([](const auto& a, const auto& b) { return a.score > b.score; });

    return MA_OK;
}

}  // namespace ma::model
