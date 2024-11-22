#ifndef _MA_MODEL_POSE_DETECTOR_H_
#define _MA_MODEL_POSE_DETECTOR_H_

#include <vector>

#include "ma_model_base.h"

namespace ma::model {

class PoseDetector : public Model {
protected:
    ma_tensor_t input_;
    ma_img_t img_;
    const ma_img_t* input_img_;

    float threshold_nms_;
    float threshold_score_;

    bool is_nhwc_;

    std::forward_list<ma_keypoint3f_t> results_;

protected:
    ma_err_t preprocess() override;

public:
    PoseDetector(Engine* engine, const char* name, ma_model_type_t type);
    virtual ~PoseDetector();

    const std::forward_list<ma_keypoint3f_t>& getResults() const;

    ma_err_t run(const ma_img_t* img);

    const void* getInput() override;

    ma_err_t setConfig(ma_model_cfg_opt_t opt, ...) override;

    ma_err_t getConfig(ma_model_cfg_opt_t opt, ...) override;
};

}  // namespace ma::model

#endif  // _MA_MODEL_POSE_DETECTOR_H
