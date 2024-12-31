#include <algorithm>

#include "ma_model_classifier.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::classifier";

Classifier::Classifier(Engine* p_engine) : Model(p_engine, "IMCLS", MA_INPUT_TYPE_IMAGE | MA_OUTPUT_TYPE_CLASS | MA_MODEL_TYPE_IMCLS) {
    input_           = p_engine_->getInput(0);
    output_          = p_engine_->getOutput(0);
    threshold_score_ = 0.5f;
    is_nhwc_         = input_.shape.dims[3] == 3 || input_.shape.dims[3] == 1;

    if (is_nhwc_) {
        img_.width  = input_.shape.dims[1];
        img_.height = input_.shape.dims[2];
        img_.size   = input_.shape.dims[1] * input_.shape.dims[2] * input_.shape.dims[3];
        img_.format = input_.shape.dims[3] == 3 ? MA_PIXEL_FORMAT_RGB888 : MA_PIXEL_FORMAT_GRAYSCALE;
    }

    else {
        img_.width  = input_.shape.dims[3];
        img_.height = input_.shape.dims[2];
        img_.size   = input_.shape.dims[3] * input_.shape.dims[2] * input_.shape.dims[1];
        img_.format = input_.shape.dims[1] == 3 ? MA_PIXEL_FORMAT_RGB888 : MA_PIXEL_FORMAT_GRAYSCALE;
    }

    img_.data = input_.data.u8;
};

Classifier::~Classifier() {}

bool Classifier::isValid(Engine* engine) {

    const auto inputs_count  = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();

    if (inputs_count != 1 || outputs_count != 1) {
        return false;
    }

    const auto input_shape = engine->getInputShape(0);
    int n = input_shape.dims[0], h = input_shape.dims[1], w = input_shape.dims[2], c = input_shape.dims[3];
    bool is_nhwc = c == 3 || c == 1;

    if (!is_nhwc)
        std::swap(h, c);

    if (n != 1 || h < 32 || h % 32 != 0 || (c != 3 && c != 1))
        return false;

    const auto output_shape = engine->getOutputShape(0);
#if MA_USE_ENGINE_HAILO
    if (output_shape.size != 2) {
        return false;
    }
#endif

    if (output_shape.dims[0] != 1 ||  // N = 1
        output_shape.dims[1] < 2      // C >= 2
    ) {
        return false;
    }

#if MA_USE_ENGINE_CVI
   if (output_shape.size == 4 && (output_shape.dims[2] != 1 || output_shape.dims[3] != 1)) {
        return false;
    }
#endif
    return true;
}


ma_err_t Classifier::preprocess() {
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

ma_err_t Classifier::postprocess() {
    results_.clear();

    switch (output_.type) {
        case MA_TENSOR_TYPE_S8: {
            auto scale{output_.quant_param.scale};
            auto zero_point{output_.quant_param.zero_point};
            bool rescale{scale < 0.1f ? true : false};
            auto* data = output_.data.s8;
            auto pred_l{output_.shape.dims[1]};

            for (decltype(pred_l) i{0}; i < pred_l; ++i) {
                auto score{static_cast<decltype(scale)>(data[i] - zero_point) * scale};
                score = rescale ? score : score / 100.f;
                if (score > threshold_score_)
                    results_.emplace_front(ma_class_t{score, i});
            }
        } break;

        case MA_TENSOR_TYPE_U8: {
            auto scale{output_.quant_param.scale};
            auto zero_point{output_.quant_param.zero_point};
            bool rescale{scale < 0.1f ? true : false};
            auto* data = output_.data.u8;
            auto pred_l{output_.shape.dims[1]};

            for (decltype(pred_l) i{0}; i < pred_l; ++i) {
                auto score{static_cast<decltype(scale)>(data[i] - zero_point) * scale};
                score = rescale ? score : score / 100.f;
                if (score > threshold_score_)
                    results_.emplace_front(ma_class_t{score, i});
            }
        } break;

        case MA_TENSOR_TYPE_U16: {
            auto scale{output_.quant_param.scale};
            auto zero_point{output_.quant_param.zero_point};
            bool rescale{scale < 0.1f ? true : false};
            auto* data = output_.data.u16;
            auto pred_l{output_.shape.dims[1]};

            for (decltype(pred_l) i{0}; i < pred_l; ++i) {
                auto score{static_cast<decltype(scale)>(data[i] - zero_point) * scale};
                score = rescale ? score : score / 100.f;
                if (score > threshold_score_)
                    results_.emplace_front(ma_class_t{score, i});
            }
        } break;

    
        case MA_TENSOR_TYPE_F32: {
            auto* data = output_.data.f32;
            auto pred_l{output_.shape.dims[1]};
            for (decltype(pred_l) i{0}; i < pred_l; ++i) {
                auto score{data[i]};
                if (score > threshold_score_)
                    results_.emplace_front(ma_class_t{score, i});
            }
        } break;

        default:
            return MA_ENOTSUP;
    }

    results_.sort([](const ma_class_t& a, const ma_class_t& b) { return a.score > b.score; });

    return MA_OK;
}


const std::forward_list<ma_class_t>& Classifier::getResults() {
    return results_;
}

const void* Classifier::getInput() {
    return &img_;
}

ma_err_t Classifier::run(const ma_img_t* img) {

    input_img_ = img;

    return underlyingRun();
}


ma_err_t Classifier::setConfig(ma_model_cfg_opt_t opt, ...) {
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
ma_err_t Classifier::getConfig(ma_model_cfg_opt_t opt, ...) {
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