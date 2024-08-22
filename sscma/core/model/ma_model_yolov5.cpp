#include <algorithm>
#include <forward_list>
#include <vector>

#include "core/utils/ma_nms.h"

#include "ma_model_yolov5.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::yolo";

YoloV5::YoloV5(Engine* p_engine_) : Detector(p_engine_, "yolov5", MA_MODEL_TYPE_YOLOV5) {
    MA_ASSERT(p_engine_ != nullptr);

    output_ = p_engine_->getOutput(0);

    num_record_  = output_.shape.dims[1];
    num_element_ = output_.shape.dims[2];
    num_class_   = num_element_ - INDEX_T;
}

YoloV5::~YoloV5() {}

bool YoloV5::isValid(Engine* engine) {

    const auto inputs_count = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();

    if (inputs_count != 1 || outputs_count != 1) {
        return false;
    }
    const auto& input_shape  = engine->getInputShape(0);
    const auto& output_shape = engine->getOutputShape(0);

    // Validate input shape
    if (input_shape.size != 4)
        return false;

    int n = input_shape.dims[0], h = input_shape.dims[1], w = input_shape.dims[2],
        c        = input_shape.dims[3];
    bool is_nhwc = c == 3 || c == 1;

    if (!is_nhwc)
        std::swap(h, c);

    if (n != 1 || h < 32 || h % 32 != 0 || (c != 3 && c != 1))
        return false;

    // Calculate expected output size based on input
    int s = w >> 5, m = w >> 4, l = w >> 3;
    int ibox_len = (s * s + m * m + l * l) * c;

    // Validate output shape
    if (output_shape.size != 3 && output_shape.size != 4)
        return false;

    if (output_shape.dims[0] != 1 || output_shape.dims[1] != ibox_len || output_shape.dims[2] < 6 ||
        output_shape.dims[2] > 85)
        return false;

    return true;
}

ma_err_t YoloV5::postprocess() {
    results_.clear();

    if (output_.type == MA_TENSOR_TYPE_S8) {
        auto* data      = output_.data.s8;
        auto scale      = output_.quant_param.scale;
        auto zero_point = output_.quant_param.zero_point;
        bool normalized = scale < 0.1f;

        for (decltype(num_record_) i = 0; i < num_record_; ++i) {
            auto idx = i * num_element_;

            auto score = static_cast<float>(data[idx + INDEX_S] - zero_point) * scale;
            score      = normalized ? score : score / 100.0f;

            if (score <= threshold_score_)
                continue;

            int8_t max_class = -128;
            int target       = 0;
            for (decltype(num_class_) t = 0; t < num_class_; ++t) {
                if (max_class < data[idx + INDEX_T + t]) {
                    max_class = data[idx + INDEX_T + t];
                    target    = t;
                }
            }

            float x = ((data[idx + INDEX_X] - zero_point) * scale);
            float y = ((data[idx + INDEX_Y] - zero_point) * scale);
            float w = ((data[idx + INDEX_W] - zero_point) * scale);
            float h = ((data[idx + INDEX_H] - zero_point) * scale);

            if (!normalized) {
                x /= img_.width;
                y /= img_.height;
                w /= img_.width;
                h /= img_.height;
            }

            ma_bbox_t box{.x      = MA_CLIP(x, 0, 1.0f),
                          .y      = MA_CLIP(y, 0, 1.0f),
                          .w      = MA_CLIP(w, 0, 1.0f),
                          .h      = MA_CLIP(h, 0, 1.0f),
                          .score  = score,
                          .target = target};

            results_.emplace_front(box);
        }
    } else if (output_.type == MA_TENSOR_TYPE_F32) {
        auto* data      = output_.data.f32;
        bool normalized = data[0] < 1.0f;

        for (decltype(num_record_) i = 0; i < num_record_; ++i) {
            auto idx   = i * num_element_;
            auto score = normalized ? data[idx + INDEX_S] : data[idx + INDEX_S] / 100.0f;

            if (score <= threshold_score_)
                continue;

            float max_class = -1.0f;
            int target      = 0;
            for (decltype(num_class_) t = 0; t < num_class_; ++t) {
                if (max_class < data[idx + INDEX_T + t]) {
                    max_class = data[idx + INDEX_T + t];
                    target    = t;
                }
            }

            float x = data[idx + INDEX_X];
            float y = data[idx + INDEX_Y];
            float w = data[idx + INDEX_W];
            float h = data[idx + INDEX_H];

            if (!normalized) {
                x /= img_.width;
                y /= img_.height;
                w /= img_.width;
                h /= img_.height;
            }

            ma_bbox_t box{.x      = MA_CLIP(x, 0, 1.0f),
                          .y      = MA_CLIP(y, 0, 1.0f),
                          .w      = MA_CLIP(w, 0, 1.0f),
                          .h      = MA_CLIP(h, 0, 1.0f),
                          .score  = score,
                          .target = target};

            results_.emplace_front(box);
        }
    } else {
        return MA_ENOTSUP;
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, false);

    results_.sort([](const ma_bbox_t& a, const ma_bbox_t& b) { return a.x < b.x; });

    return MA_OK;
}
}  // namespace ma::model
