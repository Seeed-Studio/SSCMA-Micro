#include <algorithm>

#include "ma_model_classifier.h"

namespace ma::model {

const static char* TAG = "ma::model::classifier";

Classifier::Classifier(Engine* p_engine) : Model(p_engine, "classifier") {
    input_            = p_engine_->get_input(0);
    output_           = p_engine_->get_output(0);
    threshold_score_  = 0.5f;
    input_img_.width  = input_.shape.dims[1];
    input_img_.height = input_.shape.dims[2];
    input_img_.size   = input_.shape.dims[1] * input_.shape.dims[2] * input_.shape.dims[3];
    input_img_.data   = input_.data.u8;
    if (input_.shape.dims[3] == 3) {
        input_img_.format = MA_PIXEL_FORMAT_RGB888;
    } else if (input_.shape.dims[3] == 1) {
        input_img_.format = MA_PIXEL_FORMAT_GRAYSCALE;
    }
};

Classifier::~Classifier() {}

bool Classifier::is_valid(Engine* engine) {

    const auto& input_shape = engine->get_input_shape(0);

#if MA_ENGINE_TENSOR_SHAPE_ODER_NHWC
    if (input_shape.size != 4 ||      // N, H, W, C
        input_shape.dims[0] != 1 ||   // N = 1
        input_shape.dims[1] < 16 ||   // H >= 16
        input_shape.dims[2] < 16 ||   // W >= 16
        (input_shape.dims[3] != 3 &&  // C = RGB or Gray
         input_shape.dims[3] != 1))
        return false;
#endif
#if MA_ENGINE_TENSOR_SHAPE_ODER_NCHW
    if (input_shape.size != 4 ||      // N, C, H, W
        input_shape.dims[0] != 1 ||   // N = 1
        input_shape.dims[2] < 16 ||   // H >= 16
        input_shape.dims[3] < 16 ||   // W >= 16
        (input_shape.dims[1] != 3 &&  // C = RGB or Gray
         input_shape.dims[1] != 1))
        return false;
#endif


    const auto& output_shape{engine->get_output_shape(0)};
    if (output_shape.size != 2 ||     // N, C
        output_shape.dims[0] != 1 ||  // N = 1
        output_shape.dims[1] < 2      // C >= 2
    )
        return false;

    return true;
}


ma_err_t Classifier::preprocess(const void* input) {

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

ma_err_t Classifier::postprocess() {
    results_.clear();

    if (output_.type == MA_TENSOR_TYPE_S8) {
        auto  scale{output_.quant_param.scale};
        auto  zero_point{output_.quant_param.zero_point};
        bool  rescale{scale < 0.1f ? true : false};
        auto* data = output_.data.s8;

        auto  pred_l{output_.shape.dims[1]};

        for (decltype(pred_l) i{0}; i < pred_l; ++i) {
            auto score{static_cast<decltype(scale)>(data[i] - zero_point) * scale};
            score = rescale ? score : score / 100.f;
            if (score > threshold_score_)
                results_.push_back({i, score});
        }
    } else {
        return MA_ENOTSUP;
    }

    std::sort(results_.begin(), results_.end(), [](const ma_class_t& a, const ma_class_t& b) {
        return a.score > b.score;
    });

    return MA_OK;
}


const std::vector<ma_class_t>& Classifier::get_results() {
    return results_;
}

float Classifier::get_config() {

    return threshold_score_;
}


ma_err_t Classifier::run(const ma_img_t* img) {

    return underlying_run(img);
}
ma_err_t Classifier::configure(float config) {
    threshold_score_ = config;
    return MA_OK;
}

}  // namespace ma::model