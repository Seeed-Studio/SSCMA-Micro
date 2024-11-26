#include "ma_model_point_detector.h"

#include "../cv/ma_cv.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::point_detecor";

PointDetector::PointDetector(Engine* p_engine, const char* name, ma_model_type_t type) : Model(p_engine, name, MA_INPUT_TYPE_IMAGE | MA_OUTPUT_TYPE_POINT | type) {
    input_           = p_engine_->getInput(0);
    threshold_score_ = 0.25;

    is_nhwc_ = input_.shape.dims[3] == 3 || input_.shape.dims[3] == 1;

    if (is_nhwc_) {
        img_.height = input_.shape.dims[1];
        img_.width  = input_.shape.dims[2];
        img_.size   = input_.shape.dims[1] * input_.shape.dims[2] * input_.shape.dims[3];
        img_.format = input_.shape.dims[3] == 3 ? MA_PIXEL_FORMAT_RGB888 : MA_PIXEL_FORMAT_GRAYSCALE;

    } else {
        img_.height = input_.shape.dims[2];
        img_.width  = input_.shape.dims[3];
        img_.size   = input_.shape.dims[3] * input_.shape.dims[2] * input_.shape.dims[1];
        img_.format = input_.shape.dims[1] == 3 ? MA_PIXEL_FORMAT_RGB888 : MA_PIXEL_FORMAT_GRAYSCALE;
    }

    img_.data = input_.data.u8;
}

PointDetector::~PointDetector() {}

const std::forward_list<ma_point_t>& PointDetector::getResults() const {
    return results_;
}

ma_err_t PointDetector::preprocess() {
    ma_err_t ret = MA_OK;


    if (input_img_ == nullptr) {
        return MA_OK;
    }

    ret = ma::cv::convert(input_img_, &img_);
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

ma_err_t PointDetector::run(const ma_img_t* img) {

    input_img_ = img;

    return underlyingRun();
}

const void* PointDetector::getInput() {
    return static_cast<const void*>(&img_);
}

ma_err_t PointDetector::setConfig(ma_model_cfg_opt_t opt, ...) {
    ma_err_t ret = MA_OK;
    va_list args;
    va_start(args, opt);
    switch (opt) {
        case MA_MODEL_CFG_OPT_THRESHOLD:
            threshold_score_ = va_arg(args, double);
            ret              = MA_OK;
            break;

        default:
            ret = MA_EINVAL;
            break;
    }
    va_end(args);
    return ret;
}

ma_err_t PointDetector::getConfig(ma_model_cfg_opt_t opt, ...) {
    ma_err_t ret = MA_OK;
    va_list args;
    void* p_arg = nullptr;
    va_start(args, opt);
    switch (opt) {
        case MA_MODEL_CFG_OPT_THRESHOLD:
            p_arg                          = va_arg(args, void*);
            *(static_cast<double*>(p_arg)) = threshold_score_;
            break;

        default:
            ret = MA_EINVAL;
            break;
    }
    va_end(args);
    return ret;
}

}  // namespace ma::model
