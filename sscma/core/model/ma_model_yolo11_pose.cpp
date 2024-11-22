#include "ma_model_yolo11_pose.h"

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

Yolo11Pose::Yolo11Pose(Engine* p_engine_) : PoseDetector(p_engine_, "yolo11_pose", MA_MODEL_TYPE_YOLO11_POSE) {
    MA_ASSERT(p_engine_ != nullptr);

    outputs_ = p_engine_->getOutput(0);

    num_class_     = 1;  // only one class supported
    num_record_    = outputs_.shape.dims[2];
    num_keypoints_ = (outputs_.shape.dims[1] - 5) / 3;
    num_element_   = outputs_.shape.dims[1];
}

Yolo11Pose::~Yolo11Pose() {}

bool Yolo11Pose::isValid(Engine* engine) {

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
    int ibox_len = (s * s + m * m + l * l);

    // Validate output shape
    if (output_shape.size != 3 && output_shape.size != 4)
        return false;

    if (output_shape.dims[0] != 1 || output_shape.dims[2] != ibox_len || output_shape.dims[1] < 6)
        return false;

    if ((output_shape.dims[1] - 5) % 3 != 0)
        return false;

    return true;
}

ma_err_t Yolo11Pose::postprocess() {
    results_.clear();
    if (outputs_.type == MA_TENSOR_TYPE_F32) {
        return postProcessF32();
    } else if (outputs_.type == MA_TENSOR_TYPE_S8) {
        return postProcessI8();
    }
    return MA_ENOTSUP;
}

ma_err_t Yolo11Pose::postProcessI8() {

    const float score_threshold_non_sigmoid = ma::math::inverseSigmoid(threshold_score_);

    std::forward_list<ma_bbox_ext_t> multi_level_bboxes;

    auto* data = outputs_.data.f32;
    for (decltype(num_record_) i = 0; i < num_record_; ++i) {
        auto score = data[i + num_record_ * 4];

        if (score <= score_threshold_non_sigmoid)
            continue;

        float x = ma::math::dequantizeValue(data[i], outputs_.quant_param.scale, outputs_.quant_param.zero_point);
        float y = ma::math::dequantizeValue(data[i + num_record_], outputs_.quant_param.scale, outputs_.quant_param.zero_point);
        float w = ma::math::dequantizeValue(data[i + num_record_ * 2], outputs_.quant_param.scale, outputs_.quant_param.zero_point);
        float h = ma::math::dequantizeValue(data[i + num_record_ * 3], outputs_.quant_param.scale, outputs_.quant_param.zero_point);

        ma_bbox_ext_t bbox;
        bbox.level  = 0;
        bbox.index  = i;
        bbox.x      = x / img_.width;
        bbox.y      = y / img_.height;
        bbox.w      = w / img_.width;
        bbox.h      = h / img_.height;
        bbox.score  = ma::math::dequantizeValue(score, outputs_.quant_param.scale, outputs_.quant_param.zero_point);
        bbox.target = 0;

        multi_level_bboxes.emplace_front(std::move(bbox));
    }

    ma::utils::nms(multi_level_bboxes, threshold_nms_, threshold_score_, false, true);

    if (multi_level_bboxes.empty()) {
        return MA_OK;
    }

    std::vector<ma_pt3f_t> n_keypoint(num_keypoints_);

    for (auto& bbox : multi_level_bboxes) {

        for (int i = 0; i < num_keypoints_; ++i) {
            auto index      = bbox.index + num_record_ * (5 + i * 3);
            n_keypoint[i].x = ma::math::dequantizeValue(data[index], outputs_.quant_param.scale, outputs_.quant_param.zero_point) / img_.width;
            n_keypoint[i].y = ma::math::dequantizeValue(data[index + num_record_], outputs_.quant_param.scale, outputs_.quant_param.zero_point) / img_.height;
            n_keypoint[i].z = ma::math::dequantizeValue(data[index + num_record_ * 2], outputs_.quant_param.scale, outputs_.quant_param.zero_point);
        }

        ma_keypoint3f_t keypoint;
        keypoint.box = {.x = bbox.x, .y = bbox.y, .w = bbox.w, .h = bbox.h, .score = bbox.score, .target = bbox.target};
        keypoint.pts = n_keypoint;


        results_.emplace_front(std::move(keypoint));
    }

    return MA_OK;
}
ma_err_t Yolo11Pose::postProcessF32() {

    std::forward_list<ma_bbox_ext_t> multi_level_bboxes;

    auto* data = outputs_.data.f32;
    for (decltype(num_record_) i = 0; i < num_record_; ++i) {
        auto score = data[i + num_record_ * 4];

        if (score <= threshold_score_)
            continue;

        float x = data[i];
        float y = data[i + num_record_];
        float w = data[i + num_record_ * 2];
        float h = data[i + num_record_ * 3];

        ma_bbox_ext_t bbox;
        bbox.level  = 0;
        bbox.index  = i;
        bbox.x      = x / img_.width;
        bbox.y      = y / img_.height;
        bbox.w      = w / img_.width;
        bbox.h      = h / img_.height;
        bbox.score  = score;
        bbox.target = 0;

        multi_level_bboxes.emplace_front(std::move(bbox));
    }

    ma::utils::nms(multi_level_bboxes, threshold_nms_, threshold_score_, false, true);

    if (multi_level_bboxes.empty()) {
        return MA_OK;
    }

    std::vector<ma_pt3f_t> n_keypoint(num_keypoints_);

    for (auto& bbox : multi_level_bboxes) {

        for (int i = 0; i < num_keypoints_; ++i) {
            auto index      = bbox.index + num_record_ * (5 + i * 3);
            n_keypoint[i].x = data[index] / img_.width;
            n_keypoint[i].y = data[index + num_record_] / img_.height;
            n_keypoint[i].z = data[index + num_record_ * 2];
        }

        ma_keypoint3f_t keypoint;
        keypoint.box = {.x = bbox.x, .y = bbox.y, .w = bbox.w, .h = bbox.h, .score = bbox.score, .target = bbox.target};
        keypoint.pts = n_keypoint;


        results_.emplace_front(std::move(keypoint));
    }

    return MA_OK;
}

}  // namespace ma::model
