#ifndef _MA_MODEL_DETECTOR_H
#define _MA_MODEL_DETECTOR_H

#include <vector>

#include "../cv/ma_cv.h"

#include "ma_model_base.h"

namespace ma::model {

class Detector : public Model {
protected:
    ma_tensor_t input_;
    ma_img_t img_;
    const ma_img_t* input_img_;
    double threshold_nms_;
    double threshold_score_;
    bool is_nhwc_;
    std::forward_list<ma_bbox_t> results_;

protected:
    ma_err_t preprocess() override;

public:
    Detector(Engine* engine, const char* name, ma_model_type_t type);
    virtual ~Detector();
    const std::forward_list<ma_bbox_t>& getResults();
    const void* getInput() override;
    ma_err_t run(const ma_img_t* img);
    ma_err_t setConfig(ma_model_cfg_opt_t opt, ...) override;
    ma_err_t getConfig(ma_model_cfg_opt_t opt, ...) override;
};

}  // namespace ma::model

#endif  // _MA_MODEL_DETECTOR_H
