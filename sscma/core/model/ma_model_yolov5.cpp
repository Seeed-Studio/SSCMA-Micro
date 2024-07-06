#include <algorithm>
#include <forward_list>
#include <vector>

#include "core/utils/ma_nms.h"

#include "ma_model_yolov5.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::yolo";

YoloV5::YoloV5(Engine* p_engine_) : Detector(p_engine_, "yolov5") {
    MA_ASSERT(p_engine_ != nullptr);

    output_ = p_engine_->getOutput(0);

    num_record_  = output_.shape.dims[1];
    num_element_ = output_.shape.dims[2];
    num_class_   = num_element_ - INDEX_T;
}

YoloV5::~YoloV5() {}

bool YoloV5::isValid(Engine* engine) {
    const auto& input_shape{engine->getInputShape(0)};

#if MA_ENGINE_TENSOR_SHAPE_ORDER_NHWC
    if (input_shape.size != 4 ||                      // N, H, W, C
        input_shape.dims[0] != 1 ||                   // H = 1
        input_shape.dims[1] ^ input_shape.dims[2] ||  // W = H
        input_shape.dims[1] < 32 ||                   // W, H >= 32
        input_shape.dims[1] % 32 ||                   // W or H is multiply of 32
        (input_shape.dims[3] != 3 &&                  // C = RGB or Gray
         input_shape.dims[3] != 1))
        return false;

    auto ibox_len{[&]() {
        auto r{static_cast<uint16_t>(input_shape.dims[1])};
        auto s{r >> 5};  // r / 32
        auto m{r >> 4};  // r / 16
        auto l{r >> 3};  // r / 8
        return (s * s + m * m + l * l) * input_shape.dims[3];
    }()};
#endif
#if MA_ENGINE_TENSOR_SHAPE_ORDER_NCHW
    if (input_shape.size != 4 ||                      // N, C, H, W
        input_shape.dims[0] != 1 ||                   // H = 1
        input_shape.dims[2] ^ input_shape.dims[3] ||  // W = H
        input_shape.dims[2] < 32 ||                   // W, H >= 32
        input_shape.dims[2] % 32 ||                   // W or H is multiply of 32
        (input_shape.dims[1] != 3 &&                  // C = RGB or Gray
         input_shape.dims[1] != 1))
        return false;

    auto ibox_len{[&]() {
        auto r{static_cast<uint16_t>(input_shape.dims[2])};
        auto s{r >> 5};  // r / 32
        auto m{r >> 4};  // r / 16
        auto l{r >> 3};  // r / 8
        return (s * s + m * m + l * l) * input_shape.dims[1];
    }()};
#endif

    const auto& output_shape{engine->getOutputShape(0)};
    if (output_shape.size != 3 ||            // B, IB, BC...
        output_shape.dims[0] != 1 ||         // B = 1
        output_shape.dims[1] != ibox_len ||  // IB is based on input shape
        output_shape.dims[2] < 6 ||          // 6 <= BC - 5[XYWHC] <= 80 (could be larger than 80)
        output_shape.dims[2] > 85)
        return false;

    return true;
}


ma_err_t YoloV5::postprocess() {

    std::forward_list<ma_bbox_t> result;
    results_.clear();

    auto* data = output_.data.s8;

    if (output_.type == MA_TENSOR_TYPE_S8) {
        auto scale{output_.quant_param.scale};
        auto zero_point{output_.quant_param.zero_point};
        bool normalized{scale < 0.1f ? true : false};
        for (decltype(num_record_) i{0}; i < num_record_; ++i) {
            auto idx{i * num_element_};
            auto score{static_cast<decltype(scale)>(data[idx + INDEX_S] - zero_point) * scale};
            score = normalized ? score : score / 100.0f;
            if (score <= threshold_score_)
                continue;

            ma_bbox_t box{
                .x      = 0,
                .y      = 0,
                .w      = 0,
                .h      = 0,
                .score  = score,
                .target = 0,
            };

            // get box target
            int8_t max{-128};
            for (decltype(num_class_) t{0}; t < num_class_; ++t) {
                if (max < data[idx + INDEX_T + t]) {
                    max        = data[idx + INDEX_T + t];
                    box.target = t;
                }
            }

            // get box position, int8_t - int32_t (narrowing)
            auto x{((data[idx + INDEX_X] - zero_point) * scale)};
            auto y{((data[idx + INDEX_Y] - zero_point) * scale)};
            auto w{((data[idx + INDEX_W] - zero_point) * scale)};
            auto h{((data[idx + INDEX_H] - zero_point) * scale)};

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
            result.emplace_front(std::move(box));
        }
    } else if (output_.type == MA_TENSOR_TYPE_F32) {
        auto* data      = output_.data.f32;
        bool normalized = data[0] < 1.0f ? true : false;
        for (decltype(num_record_) i{0}; i < num_record_; ++i) {
            auto idx{i * num_element_};
            auto score = normalized ? data[idx + INDEX_S] : data[idx + INDEX_S] / 100.0f;
            if (score <= threshold_score_)
                continue;

            ma_bbox_t box{
                .x      = 0,
                .y      = 0,
                .w      = 0,
                .h      = 0,
                .score  = score,
                .target = 0,
            };
            // get box target
            float max{-1.0f};
            for (decltype(num_class_) t{0}; t < num_class_; ++t) {
                if (max < data[idx + INDEX_T + t]) {
                    max        = data[idx + INDEX_T + t];
                    box.target = t;
                }
            }
            auto x{data[idx + INDEX_X]};
            auto y{data[idx + INDEX_Y]};
            auto w{data[idx + INDEX_W]};
            auto h{data[idx + INDEX_H]};

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
            result.emplace_front(std::move(box));
        }
    } else {
        return MA_ENOTSUP;
    }

    ma::utils::nms(result, threshold_nms_, threshold_score_, false, false);

    result.sort([](const ma_bbox_t& a, const ma_bbox_t& b) { return a.x < b.x; });  // left to right

    std::copy(result.begin(), result.end(), std::back_inserter(results_));

    return MA_OK;
}

}  // namespace ma::model
