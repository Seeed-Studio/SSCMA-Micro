#include "ma_model_pfld.h"

#include <algorithm>
#include <forward_list>

namespace ma::model {

PFLD::PFLD(Engine* p_engine_) : PointDetector(p_engine_, "pfld", MA_MODEL_TYPE_PFLD) {
    MA_ASSERT(p_engine_ != nullptr);
}

PFLD::~PFLD() {}

bool PFLD::isValid(Engine* engine) {
    const auto inputs_count  = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();

    if (inputs_count != 1 || outputs_count != 1) {
        return false;
    }

    const auto input_shape{engine->getInputShape(0)};
    const auto output_shape{engine->getOutputShape(0)};

    if (input_shape.size != 4) {
        return false;
    }

    const bool is_nhwc{input_shape.dims[3] == 3 || input_shape.dims[3] == 1};

    size_t n = 0, h = 0, w = 0, c = 0;

    if (is_nhwc) {
        n = input_shape.dims[0];
        h = input_shape.dims[1];
        w = input_shape.dims[2];
        c = input_shape.dims[3];
    } else {
        n = input_shape.dims[0];
        c = input_shape.dims[1];
        h = input_shape.dims[2];
        w = input_shape.dims[3];
    }

    if (n != 1 || h ^ w || h < 32 || h % 32 || (c != 3 && c != 1)) {
        return false;
    }

    if (output_shape.size != 2 ||     // B, PTS
        output_shape.dims[0] != 1 ||  // B = 1
        output_shape.dims[1] % 2      // PTS should be multiply of 2
    ) {
        return false;
    }

    return true;
}

const char* PFLD::getTag() { return "ma::model::pfld"; }

ma_err_t PFLD::postprocess() {
    const auto out = this->p_engine_->getOutput(0);

    switch (out.type) {
    case MA_TENSOR_TYPE_S8:
        return postProcessI8();

    default:
        return MA_ENOTSUP;
    }

    return MA_ENOTSUP;
}

ma_err_t PFLD::postProcessI8() {
    results_.clear();

    const auto  output = p_engine_->getOutput(0);
    const auto* data   = output.data.s8;

    float         scale      = output.quant_param.scale;
    const bool    rescale    = scale < 0.1f ? true : false;
    const int32_t zero_point = output.quant_param.zero_point;
    const auto    pred_l     = output.shape.dims[1];

    scale = rescale ? scale : scale / 100.f;

    const float w = input_img_->width;
    const float h = input_img_->height;

    for (int i = 0; i < pred_l; i += 2) {
        ma_point_t point;

        point.x      = (data[i] - zero_point) * scale / w;
        point.y      = (data[i + 1] - zero_point) * scale / h;
        point.score  = 1.0;
        point.target = i / 2;

        results_.push_front(std::move(point));
    }

    return MA_OK;
}

}  // namespace ma::model
