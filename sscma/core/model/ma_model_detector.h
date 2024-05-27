#ifndef _MA_MODEL_DETECTOR_H
#define _MA_MODEL_DETECTOR_H

#include <vector>

#include "core/cv/ma_cv.h"

#include "core/model/ma_model_base.h"


namespace ma::model {
class Detector : public Model {
protected:
    ma_tensor_t            input_;
    ma_img_t               input_img_;
    ma_detect_cfg_t        config_;
    std::vector<ma_bbox_t> results_;

protected:
    ma_err_t preprocess(const void* input) override;

public:
    Detector(Engine* engine, const char* name);
    virtual ~Detector();
    const std::vector<ma_bbox_t>& get_results();
    ma_detect_cfg_t               get_config();
    ma_err_t                      run(const ma_img_t* img);
    ma_err_t                      configure(const ma_detect_cfg_t config);
};

}  // namespace ma::model


#endif  // _MA_MODEL_DETECTOR_H