#ifndef _MA_MODEL_YOLO_H
#define _MA_MODEL_YOLO_H

#include <vector>

#include "core/model/ma_model_base.h"


namespace ma::model {
class Yolo : public Model {
private:
    ma_tensor_t            input_;
    ma_tensor_t            output_;
    ma_img_t               input_img_;
    std::vector<ma_bbox_t> results_;
    enum {
        INDEX_X = 0,
        INDEX_Y = 1,
        INDEX_W = 2,
        INDEX_H = 3,
        INDEX_S = 4,
        INDEX_T = 5,
    };

protected:
    bool     is_valid() override;
    ma_err_t preprocess(const void* input) override;
    ma_err_t postprocess() override;

public:
    Yolo(Engine* engine);
    ~Yolo();
    ma_err_t                      run(const void* input) override;
    std::vector<ma_bbox_t>& get_results();
};

}  // namespace ma::model


#endif  // _MA_MODEL_YOLO_H