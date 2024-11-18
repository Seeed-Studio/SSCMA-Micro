#include "ma_model_nvidia_det.h"

#include <algorithm>
#include <forward_list>
#include <vector>

#include "../utils/ma_nms.h"

namespace ma::model {

NvidiaDet::NvidiaDet(Engine* p_engine_)
    : Detector(p_engine_, "nvidia_det", MA_MODEL_TYPE_NVIDIA_DET) {
    MA_ASSERT(p_engine_ != nullptr);
}

NvidiaDet::~NvidiaDet() {}

bool NvidiaDet::isValid(Engine* engine) {
    const auto inputs_count  = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();

    if (inputs_count != 1 || outputs_count != 2) {
        return false;
    }

    const auto input_shape{engine->getInputShape(0)};
    const auto output_shape_0{engine->getOutputShape(0)};
    const auto output_shape_1{engine->getOutputShape(1)};

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

    if (output_shape_0.size < 4 || output_shape_1.size < 4) {
        return false;
    }

    if (input_shape.dims[1] / 16 != output_shape_0.dims[1] ||
        input_shape.dims[2] / 16 != output_shape_0.dims[2] ||
        input_shape.dims[1] / 16 != output_shape_1.dims[1] ||
        input_shape.dims[2] / 16 != output_shape_1.dims[2]) {
        return false;
    }

    return true;
}

const char* NvidiaDet::getTag() {
    return "ma::model::nvidia_det";
}

ma_err_t NvidiaDet::postprocess() {
    uint8_t check = 0;

    for (size_t i = 0; i < 2; ++i) {
        const auto out = this->p_engine_->getOutput(i);

        switch (out.type) {
            case MA_TENSOR_TYPE_F32:
                check |= 1 << i;
                break;

            default:
                return MA_ENOTSUP;
        }
    }

    switch (check) {
        case 0b11:
            return postProcessF32();

        default:
            return MA_ENOTSUP;
    }
}

ma_err_t NvidiaDet::postProcessF32() {
    results_.clear();

    // get output
    const auto out0 = p_engine_->getOutput(0);
    const auto out1 = p_engine_->getOutput(1);

    const auto* data0 = out0.data.f32;
    const auto* data1 = out1.data.f32;

    const auto output_shape0 = out0.shape;
    const auto output_shape1 = out1.shape;

    const auto* bboxs = output_shape0.dims[3] > output_shape1.dims[3] ? data0 : data1;
    const auto* conf  = output_shape0.dims[3] > output_shape1.dims[3] ? data1 : data0;

    conf_shape_   = output_shape0.dims[3] > output_shape1.dims[3] ? output_shape1 : output_shape0;
    bboxes_shape_ = output_shape0.dims[3] > output_shape1.dims[3] ? output_shape0 : output_shape1;

    const auto H = conf_shape_.dims[1];
    const auto W = conf_shape_.dims[2];
    const auto N = conf_shape_.dims[3];
    const auto C = N * 4;

    for (int h = 0; h < H; h++) {
        for (int w = 0; w < W; w++) {
            for (int j = 0; j < N; j++) {
                if (conf[h * (W * N) + w * N + j] > 0.2) {
                    ma_bbox_t box;

                    box.x = (w * stride_ + offset_ - bboxs[h * (W * C) + w * C + j * 4] * scale_) /
                        img_.width;
                    box.y =
                        (h * stride_ + offset_ - bboxs[h * (W * C) + w * C + j * 4 + 1] * scale_) /
                        img_.height;

                    box.w =
                        ((w * stride_ + offset_ + bboxs[h * (W * C) + w * C + j * 4 + 2] * scale_) /
                         img_.width) -
                        box.x;
                    box.h =
                        ((h * stride_ + offset_ + bboxs[h * (W * C) + w * C + j * 4 + 3] * scale_) /
                         img_.height) -
                        box.y;

                    box.x = box.x + box.w / 2;
                    box.y = box.y + box.h / 2;

                    box.score  = conf[h * (W * N) + w * N + j] * 2.0;
                    box.target = j;

                    results_.emplace_front(std::move(box));
                }
            }
        }
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, true);

    results_.sort([](const ma_bbox_t& a, const ma_bbox_t& b) { return a.x < b.x; });

    return MA_OK;
}

}  // namespace ma::model
