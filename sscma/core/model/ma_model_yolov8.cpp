#include "ma_model_yolov8.h"

#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/math/ma_math.h"
#include "core/utils/ma_nms.h"

namespace ma::model {

YoloV8::YoloV8(Engine* p_engine_) : Detector(p_engine_, "yolov8", MA_MODEL_TYPE_YOLOV8) {
    MA_ASSERT(p_engine_ != nullptr);

    output_ = p_engine_->getOutput(0);

    num_record_  = output_.shape.dims[1];
    num_element_ = output_.shape.dims[2];
    num_class_   = num_element_ - INDEX_T;
}

YoloV8::~YoloV8() {}

bool YoloV8::isValid(Engine* engine) {
    const auto inputs_count = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();

    if (inputs_count != 1 || outputs_count != 1) {
        return false;
    }

    const auto input_shape{engine->getInputShape(0)};
    const auto output_shape{engine->getOutputShape(0)};

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

    const auto ibox_len{[&]() {
        auto s{w >> 5};  // r / 32
        auto m{w >> 4};  // r / 16
        auto l{w >> 3};  // r / 8
        return (s * s + m * m + l * l) * c;
    }()};

    if (output_shape.size != 3 ||            // B, IB, BC...
        output_shape.dims[0] != 1 ||         // B = 1
        output_shape.dims[2] != ibox_len ||  // IB is based on input shape
        output_shape.dims[1] < 5 ||          // 6 <= BC - 5[XYWH] <= 80 (could be larger than 80)
        output_shape.dims[1] > 84) {
        return false;
    }

    return true;
}

const char* YoloV8::getTag() { return "ma::model::yolov8"; }

ma_err_t YoloV8::postprocess() {
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

ma_err_t YoloV8::postProcessI8() {
    results_.clear();

    const auto* data = output_.data.s8;

    const auto scale{output_.quant_param.scale};
    const auto zero_point{output_.quant_param.zero_point};
    const bool normalized{(scale * (std::numeric_limits<int8_t>::max() - std::numeric_limits<int8_t>::min())) <= 1.0};

    for (decltype(num_record_) i = 0; i < num_record_; ++i) {
        uint16_t target = 0;
        auto     max    = std::numeric_limits<int8_t>::min();

        for (decltype(num_class_) t = 0; t < num_class_; ++t) {
            const auto n = data[i + (t + INDEX_T) * num_record_];
            if (max < n) {
                max    = n;
                target = t;
            }
        }

        float score =
          ma::math::dequantizeValue(static_cast<int32_t>(data[i + INDEX_S * num_record_]), scale, zero_point);
        score = normalized ? score : std::round(score / 100.f);

        if (score > threshold_score_) {
            ma_bbox_t box;

            box.score  = score;
            box.target = target;

            // get box position, int8_t - int32_t (narrowing)
            float x =
              ma::math::dequantizeValue(static_cast<int32_t>(data[i + INDEX_X * num_record_]), scale, zero_point);
            float y =
              ma::math::dequantizeValue(static_cast<int32_t>(data[i + INDEX_Y * num_record_]), scale, zero_point);
            float w =
              ma::math::dequantizeValue(static_cast<int32_t>(data[i + INDEX_W * num_record_]), scale, zero_point);
            float h =
              ma::math::dequantizeValue(static_cast<int32_t>(data[i + INDEX_H * num_record_]), scale, zero_point);

            if (!normalized) {
                x = x / img_.width;
                y = y / img_.height;
                w = w / img_.width;
                h = h / img_.height;
            }

            results_.push_back(std::move(box));
        }
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, num_class_ > 1);

    results_.shrink_to_fit();

    return MA_OK;
}

ma_err_t YoloV8::postProcessF32() {
    results_.clear();

    const auto* data = output_.data.f32;

    for (decltype(num_record_) i = 0; i < num_record_; ++i) {
        uint16_t target = 0;
        auto     max    = std::numeric_limits<float>::min();

        for (decltype(num_class_) t = 0; t < num_class_; ++t) {
            const auto n = data[i + (t + INDEX_T) * num_record_];
            if (max < n) {
                max    = n;
                target = t;
            }
        }

        const float score{max};

        if (score >= threshold_score_) {
            ma_bbox_t box;

            box.score  = score;
            box.target = target;

            // get box position, int8_t - int32_t (narrowing)
            float x = data[i + INDEX_X * num_record_];
            float y = data[i + INDEX_Y * num_record_];
            float w = data[i + INDEX_W * num_record_];
            float h = data[i + INDEX_H * num_record_];

            results_.push_back(std::move(box));
        }
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, num_class_ > 1);

    results_.shrink_to_fit();

    return MA_OK;
}

}  // namespace ma::model
