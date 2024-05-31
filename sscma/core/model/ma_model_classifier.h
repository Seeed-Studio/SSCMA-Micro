#ifndef _MA_MODEL_CLASSIFIER_H
#define _MA_MODEL_CLASSIFIER_H

#include <vector>

#include "core/cv/ma_cv.h"

#include "core/model/ma_model_base.h"


namespace ma::model {

class Classifier : public Model {
protected:
    ma_tensor_t             input_;
    ma_tensor_t             output_;
    ma_img_t                img_;
    const ma_img_t*         input_img_;
    double                  threshold_score_;
    std::vector<ma_class_t> results_;

protected:
    ma_err_t preprocess() override;
    ma_err_t postprocess() override;

public:
    Classifier(Engine* engine);
    virtual ~Classifier();
    bool                           is_valid(Engine* engine) override;
    const std::vector<ma_class_t>& get_results();
    ma_err_t                       run(const ma_img_t* img);
    ma_err_t                       set_config(ma_model_cfg_opt_t opt, ...) override;
    ma_err_t                       get_config(ma_model_cfg_opt_t opt, ...) override;
};

}  // namespace ma::model


#endif  // _MA_MODEL_DETECTOR_H