#ifndef _MA_MODEL_YOLO11_POSE_H_
#define _MA_MODEL_YOLO11_POSE_H_

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "ma_model_pose_detector.h"

namespace ma::model {

class Yolo11Pose : public PoseDetector {
private:
    ma_tensor_t outputs_;
    int32_t num_record_;
    int32_t num_element_;
    int32_t num_class_;
    int32_t num_keypoints_;

protected:
    ma_err_t postprocess() override;

    ma_err_t postProcessI8();
    ma_err_t postProcessF32();

public:
    Yolo11Pose(Engine* engine);
    ~Yolo11Pose();

    static bool isValid(Engine* engine);
};

}  // namespace ma::model

#endif  // _MA_MODEL_YOLO_H
