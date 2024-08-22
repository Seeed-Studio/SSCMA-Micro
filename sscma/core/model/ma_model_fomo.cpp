#include "ma_model_fomo.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace ma::model {

FOMO::FOMO(Engine* p_engine_) : Detector(p_engine_, "fomo", MA_MODEL_TYPE_FOMO) {
    MA_ASSERT(p_engine_ != nullptr);
}

FOMO::~FOMO() {}

bool FOMO::isValid(Engine* engine) {
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

    if (output_shape.size != 4 ||     // B, W, H, PC
        output_shape.dims[0] != 1 ||  // B = 1
        output_shape.dims[1] < 2 ||   // W >= 2
        output_shape.dims[1] > w ||   // W <= IW
        output_shape.dims[2] < 2 ||   // H >= 2
        output_shape.dims[2] > h ||   // H <= IH
        output_shape.dims[3] < 2      // PC >= 2
    ) {
        return false;
    }

    return true;
}

const char* FOMO::getTag() {
    return "ma::model::fomo";
}

ma_err_t FOMO::postprocess() {
    const auto out = this->p_engine_->getOutput(0);

    switch (out.type) {
        case MA_TENSOR_TYPE_S8:
            return postProcessI8();

        default:
            return MA_ENOTSUP;
    }

    return MA_ENOTSUP;
}

ma_err_t FOMO::postProcessI8() {
    results_.clear();

    const auto output = p_engine_->getOutput(0);
    const auto* data  = output.data.s8;

    const auto width  = input_img_->width;
    const auto height = input_img_->height;

    const float scale = output.quant_param.scale;

    const int32_t zero_point = output.quant_param.zero_point;

    const auto pred_w = output.shape.dims[2];
    const auto pred_h = output.shape.dims[1];
    const auto pred_t = output.shape.dims[3];

    const auto bw = static_cast<int>(width / pred_w);
    const auto bh = static_cast<int>(height / pred_h);

    const auto score_threshold = threshold_score_;

    for (int i = 0; i < pred_h; ++i) {
        for (int j = 0; j < pred_w; ++j) {
            float max_score = score_threshold;
            int max_target  = -1;
            for (int t = 0; t < pred_t; ++t) {
                const auto score =
                    (data[i * pred_w * pred_t + j * pred_t + t] - zero_point) * scale;
                if (score > max_score) {
                    max_score  = score;
                    max_target = t;
                }
            }

            if (max_target == -1) {
                continue;
            }

            ma_bbox_t box;

            box.x      = (j * bw + (bw >> 1)) / width;
            box.y      = (i * bh + (bh >> 1)) / height;
            box.w      = bw / width;
            box.h      = bh / height;
            box.score  = max_score;
            box.target = max_target;

            results_.emplace_front(std::move(box));
        }
    }

    results_.sort([](const ma_bbox_t& a, const ma_bbox_t& b) { return a.x < b.x; });

    return MA_OK;
}

}  // namespace ma::model
