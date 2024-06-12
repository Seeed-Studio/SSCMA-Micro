#include "ma_model_detector.h"

namespace ma::model {

const static char* TAG = "ma::model::detecor";


Detector::Detector(Engine* p_engine, const char* name) : Model(p_engine, name) {
    input_           = p_engine_->getInput(0);
    threshold_nms_   = 0.45;
    threshold_score_ = 0.25;
#if MA_ENGINE_TENSOR_SHAPE_ORDER_NHWC
    img_.height = input_.shape.dims[1];
    img_.width  = input_.shape.dims[2];
    img_.size   = input_.shape.dims[1] * input_.shape.dims[2] * input_.shape.dims[3];
#endif
#if MA_ENGINE_TENSOR_SHAPE_ORDER_NCHW
    img_.height = input_.shape.dims[2];
    img_.width  = input_.shape.dims[3];
    img_.size   = input_.shape.dims[3] * input_.shape.dims[2] * input_.shape.dims[1];
#endif
    img_.data = input_.data.u8;
    if (input_.shape.dims[3] == 3) {
        img_.format = MA_PIXEL_FORMAT_RGB888;
    } else if (input_.shape.dims[3] == 1) {
        img_.format = MA_PIXEL_FORMAT_GRAYSCALE;
    }
};


ma_err_t Detector::preprocess() {

    ma_err_t ret = MA_OK;

    ret = ma::cv::convert(input_img_, &img_);
    if (ret != MA_OK) {
        return ret;
    }
    if (input_.type == MA_TENSOR_TYPE_S8) {
        for (int i = 0; i < input_.size; i++) {
            input_.data.u8[i] -= 128;
        }
    }

#if MA_ENGINE_TENSOR_SHAPE_ORDER_NCHW
    // TODO: use opencv

    if (input_.shape.dims[1] == 3) {
        char* r_channel = new char[img_.width * img_.height];
        char* g_channel = new char[img_.width * img_.height];
        char* b_channel = new char[img_.width * img_.height];
        for (int i = 0; i < img_.width * img_.height; i++) {
            r_channel[i] = img_.data[3 * i];
            g_channel[i] = img_.data[3 * i + 1];
            b_channel[i] = img_.data[3 * i + 2];
        }
        memcpy(img_.data, r_channel, img_.width * img_.height);
        memcpy(img_.data + img_.width * img_.height, g_channel, img_.width * img_.height);
        memcpy(img_.data + 2 * img_.width * img_.height, b_channel, img_.width * img_.height);
        delete[] r_channel;
        delete[] g_channel;
        delete[] b_channel;
    }
#endif
    return ret;
}

const std::vector<ma_bbox_t>& Detector::getResults() {
    return results_;
}


ma_err_t Detector::run(const ma_img_t* img) {
    MA_ASSERT(img != nullptr);
    input_img_ = img;
    return underlyingRun();
}

ma_err_t Detector::setConfig(ma_model_cfg_opt_t opt, ...) {
    ma_err_t ret = MA_OK;
    va_list  args;
    va_start(args, opt);
    switch (opt) {
        case MA_MODEL_CFG_OPT_THRESHOLD:
            threshold_score_ = va_arg(args, double);
            ret              = MA_OK;
            break;
        case MA_MODEL_CFG_OPT_NMS:
            threshold_nms_ = va_arg(args, double);
            ret            = MA_OK;
            break;
        default:
            ret = MA_EINVAL;
            break;
    }
    va_end(args);
    return ret;
}
ma_err_t Detector::getConfig(ma_model_cfg_opt_t opt, ...) {
    ma_err_t ret = MA_OK;
    va_list  args;
    void*    p_arg = nullptr;
    va_start(args, opt);
    switch (opt) {
        case MA_MODEL_CFG_OPT_THRESHOLD:
            p_arg                          = va_arg(args, void*);
            *(static_cast<double*>(p_arg)) = threshold_score_;
            break;
        case MA_MODEL_CFG_OPT_NMS:
            p_arg                          = va_arg(args, void*);
            *(static_cast<double*>(p_arg)) = threshold_nms_;
            break;
        default:
            ret = MA_EINVAL;
            break;
    }
    va_end(args);
    return ret;
}

Detector::~Detector() {}

}  // namespace ma::model