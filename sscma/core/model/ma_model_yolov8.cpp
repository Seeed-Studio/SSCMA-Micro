#include "ma_model_yolov8.h"

#include <algorithm>
#include <forward_list>
#include <vector>

#include "core/utils/ma_nms.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::yolov8";

YoloV8::YoloV8(Engine* p_engine_) : Detector(p_engine_, "yolov8", MA_MODEL_TYPE_YOLOV8) {
    MA_ASSERT(p_engine_ != nullptr);

    output_ = p_engine_->getOutput(0);

    num_record_  = output_.shape.dims[1];
    num_element_ = output_.shape.dims[2];
    num_class_   = num_element_ - INDEX_T;
}

YoloV8::~YoloV8() {}

bool YoloV8::isValid(Engine* engine) {
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

    auto ibox_len{[&]() {
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

ma_err_t YoloV8::postprocess() {
    results_.clear();

    switch (output_.type) {
    case MA_TENSOR_TYPE_S8: {
        const auto* data = output_.data.s8;

        const auto scale{output_.quant_param.scale};
        const auto zero_point{output_.quant_param.zero_point};
        const bool normalized{scale < 0.1f ? true : false};

        for (decltype(num_record_) i{0}; i < num_record_; ++i) {
            uint16_t target = 0;
            int8_t   max{INT8_MIN};

            for (decltype(num_class_) t{0}; t < num_class_; ++t) {
                const auto n = data[idx + (t + INDEX_T) * num_record];
                if (max < n) {
                    max    = n;
                    target = t;
                }
            }

            auto score{static_cast<decltype(scale)>(max - zero_point) * scale};
            score = normalized ? score * 100.f : score;

            if (score > score_threshold) {
                BoxType box{
                  .x      = 0,
                  .y      = 0,
                  .w      = 0,
                  .h      = 0,
                  .score  = static_cast<decltype(BoxType::score)>(std::round(score)),
                  .target = static_cast<decltype(BoxType::target)>(target),
                };

                // get box position, int8_t - int32_t (narrowing)
                auto x{((data[idx + INDEX_X * num_record] - zero_point) * scale)};
                auto y{((data[idx + INDEX_Y * num_record] - zero_point) * scale)};
                auto w{((data[idx + INDEX_W * num_record] - zero_point) * scale)};
                auto h{((data[idx + INDEX_H * num_record] - zero_point) * scale)};

                if (!normalized) {
                    x = x / img_.width;
                    y = y / img_.height;
                    w = w / img_.width;
                    h = h / img_.height;
                }

                box.x = MA_CLIP(x, 0, 1.0f);
                box.y = MA_CLIP(y, 0, 1.0f);
                box.w = MA_CLIP(w, 0, 1.0f);
                box.h = MA_CLIP(h, 0, 1.0f);

                results_.push_back(std::move(box));
            }
        }
    } break;

    case MA_TENSOR_TYPE_F32: {
        const auto* data = output_.data.f32;

        const auto scale{output_.quant_param.scale};
        const auto zero_point{output_.quant_param.zero_point};
        const bool normalized{scale < 0.1f ? true : false};

        for (decltype(num_record_) i{0}; i < num_record_; ++i) {
            uint16_t target = 0;
            int8_t   max{INT8_MIN};

            for (decltype(num_class_) t{0}; t < num_class_; ++t) {
                const auto n = data[idx + (t + INDEX_T) * num_record];
                if (max < n) {
                    max    = n;
                    target = t;
                }
            }

            auto score{max};
            score = normalized ? score * 100.f : score;

            if (score > score_threshold) {
                BoxType box{
                  .x      = 0,
                  .y      = 0,
                  .w      = 0,
                  .h      = 0,
                  .score  = static_cast<decltype(BoxType::score)>(std::round(score)),
                  .target = static_cast<decltype(BoxType::target)>(target),
                };

                // get box position, int8_t - int32_t (narrowing)
                auto x{data[idx + INDEX_X * num_record]};
                auto y{data[idx + INDEX_Y * num_record]};
                auto w{data[idx + INDEX_W * num_record]};
                auto h{data[idx + INDEX_H * num_record]};

                if (!normalized) {
                    x = x / img_.width;
                    y = y / img_.height;
                    w = w / img_.width;
                    h = h / img_.height;
                }

                box.x = MA_CLIP(x, 0, 1.0f);
                box.y = MA_CLIP(y, 0, 1.0f);
                box.w = MA_CLIP(w, 0, 1.0f);
                box.h = MA_CLIP(h, 0, 1.0f);

                results_.push_back(std::move(box));
            }
        }
    } break;

    default:
        return MA_ENOTSUP;
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, true);

    results_.shrink_to_fit();

    return MA_OK;
}

}  // namespace ma::model
