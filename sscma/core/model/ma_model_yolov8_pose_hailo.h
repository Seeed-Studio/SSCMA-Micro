#ifndef _MA_MODEL_YOLOV8_POSE_HAILO_H_
#define _MA_MODEL_YOLOV8_POSE_HAILO_H_

#include "ma_model_pose_detector.h"

#if MA_USE_ENGINE_HAILO

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <xtensor/xtensor.hpp>
#include <xtensor/xarray.hpp>

namespace ma::model {

namespace _internal {

struct Triple {
    std::vector<ma_tensor_t> boxes;
    xt::xarray<float> scores;
    std::vector<ma_tensor_t> keypoints;
};

}  // namespace _internal

class YoloV8PoseHailo : public PoseDetector {
private:
    std::vector<xt::xarray<double>> centers_;
    static std::vector<int> strides_;
    std::vector<int> network_dims_;
    std::vector<ma_tensor_t> outputs_;
    _internal::Triple boxes_scores_keypoints_;
    int32_t route_ = 0;

protected:
    ma_err_t postprocess() override;

public:
    YoloV8PoseHailo(Engine* engine);
    ~YoloV8PoseHailo();

    static bool isValid(Engine* engine);

    static const char* getTag();
};

}  // namespace ma::model

#endif

#endif  // _MA_MODEL_YOLO_H
