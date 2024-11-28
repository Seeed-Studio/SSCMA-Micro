#include <algorithm>
#include <forward_list>
#include <utility>
#include <vector>

#include "../utils/ma_nms.h"

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

static bool generalValid(Engine* engine) {
    const auto inputs_count  = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();

    if (inputs_count != 1 || outputs_count != 1) {
        return false;
    }
    const auto& input_shape  = engine->getInputShape(0);
    const auto& output_shape = engine->getOutputShape(0);

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
    int ibox_len = (s * s + m * m + l * l) * c;

    // Validate output shape
    if (output_shape.size != 3 && output_shape.size != 4)
        return false;

    if (output_shape.dims[0] != 1 || output_shape.dims[1] != ibox_len || output_shape.dims[2] < 6 || output_shape.dims[2] > 85)
        return false;

    return true;
}

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

bool YoloV5::isValid(Engine* engine) {
    if (!engine || engine->getOutputSize() != 1)
        return false;
    auto output = engine->getOutput(0);

    switch (output.type) {
        case MA_TENSOR_TYPE_NMS_BBOX_U16:
        case MA_TENSOR_TYPE_NMS_BBOX_F32:
            return nmsValid(engine);
        default:
            return generalValid(engine);
    }
}

ma_err_t YoloV5::generalPostProcess() {
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

            ma_bbox_t box{.x = MA_CLIP(x, 0, 1.0f), .y = MA_CLIP(y, 0, 1.0f), .w = MA_CLIP(w, 0, 1.0f), .h = MA_CLIP(h, 0, 1.0f), .score = score, .target = target};

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

            ma_bbox_t box{.x = MA_CLIP(x, 0, 1.0f), .y = MA_CLIP(y, 0, 1.0f), .w = MA_CLIP(w, 0, 1.0f), .h = MA_CLIP(h, 0, 1.0f), .score = score, .target = target};

            results_.emplace_front(box);
        }
    } else {
        return MA_ENOTSUP;
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, false);

    results_.sort([](const ma_bbox_t& a, const ma_bbox_t& b) { return a.x < b.x; });

    return MA_OK;
}

ma_err_t YoloV5::nmsPostProcess() {
#if MA_USE_ENGINE_HAILO

    auto& output = output_;

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

ma_err_t YoloV5::postprocess() {
    results_.clear();

    switch (output_.type) {
        case MA_TENSOR_TYPE_NMS_BBOX_U16:
        case MA_TENSOR_TYPE_NMS_BBOX_F32: {
#if MA_USE_ENGINE_HAILO
            // TODO: can be optimized by whihout calling this handler for each frame
            if (output_.external_handler) {
                auto ph   = reinterpret_cast<ma::engine::EngineHailo::ExternalHandler*>(output_.external_handler);
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

        default:
            return generalPostProcess();
    }


    return MA_ENOTSUP;
}
}  // namespace ma::model
