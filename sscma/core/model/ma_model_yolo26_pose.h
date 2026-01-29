#ifndef _MA_MODEL_YOLO26_POSE_H
#define _MA_MODEL_YOLO26_POSE_H

#include <vector>

#include "ma_model_pose_detector.h"

namespace ma::model {

class Yolo26Pose : public PoseDetector {
private:
    ma_tensor_t outputs_[9]; // Up to 9 for decoupled (3 scales * 3 types), or 1 for NMS
    int32_t num_tensor_;
    int32_t num_record_;
    int32_t num_class_;
    int32_t num_keypoints_;
    
    // Indices mapping
    int8_t box_idx_[3];
    int8_t cls_idx_[3];
    int8_t kpt_idx_[3];

protected:
    ma_err_t postprocess() override;

    ma_err_t postProcessI8();
    ma_err_t postProcessF32();
    ma_err_t nmsPostProcess();


public:
    Yolo26Pose(Engine* engine);
    ~Yolo26Pose();
    static bool isValid(Engine* engine);
};

}  // namespace ma::model

#endif
