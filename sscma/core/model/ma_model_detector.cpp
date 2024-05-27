#include "ma_model_detector.h"

namespace ma::model {

const static char* TAG = "ma::model::detecor";


Detector::Detector(Engine* p_engine, const char* name) : Model(p_engine, name) {
    input_                  = p_engine_->get_input(0);
    config_.threshold_nms   = 0.45;
    config_.threshold_score = 0.25;
    input_img_.width        = input_.shape.dims[1];
    input_img_.height       = input_.shape.dims[2];
    input_img_.size         = input_.shape.dims[1] * input_.shape.dims[2] * input_.shape.dims[3];
    input_img_.data         = input_.data.u8;
    if (input_.shape.dims[3] == 3) {
        input_img_.format = MA_PIXEL_FORMAT_RGB888;
    } else if (input_.shape.dims[3] == 1) {
        input_img_.format = MA_PIXEL_FORMAT_GRAYSCALE;
    }
};


ma_err_t Detector::preprocess(const void* input) {

    MA_ASSERT(input != nullptr);

    ma_err_t        ret     = MA_OK;
    const ma_img_t* src_img = static_cast<const ma_img_t*>(input);

    ret = ma::cv::convert(src_img, &input_img_);
    if (ret != MA_OK) {
        return ret;
    }
    if (input_.type == MA_TENSOR_TYPE_S8) {
        for (int i = 0; i < input_.size; i++) {
            input_.data.u8[i] -= 128;
        }
    }

    return ret;
}

const std::vector<ma_bbox_t>& Detector::get_results() {
    return results_;
}

ma_detect_cfg_t Detector::get_config() {

    return config_;
}


ma_err_t Detector::run(const ma_img_t* img) {

    return underlying_run(img);
}
ma_err_t Detector::configure(const ma_detect_cfg_t config) {
    config_.threshold_nms   = config.threshold_nms;
    config_.threshold_score = config.threshold_score;
    return MA_OK;
}


Detector::~Detector() {}

}  // namespace ma::model