#include <algorithm>
#include <forward_list>
#include <vector>

#include "core/utils/ma_nms.h"

#include "ma_model_nvidia_det.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::yolo";

NvidiaDet::NvidiaDet(Engine* p_engine_) : Detector(p_engine_, "nvidia_det", MA_MODEL_TYPE_YOLOV5) {
    MA_ASSERT(p_engine_ != nullptr);
}

NvidiaDet::~NvidiaDet() {}

bool NvidiaDet::isValid(Engine* engine) {

    const auto input_shape{engine->getInputShape(0)};
    const auto output_shape_0{engine->getOutputShape(0)};
    const auto output_shape_1{engine->getOutputShape(1)};

    auto is_nhwc{input_shape.dims[3] == 3 || input_shape.dims[3] == 1};

    auto n{0}, h{0}, w{0}, c{0};

    if (input_shape.size != 4) {
        return false;
    }

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

    if (input_shape.dims[1] / 16 != output_shape_0.dims[1] || input_shape.dims[2] / 16 != output_shape_0.dims[2] ||
        input_shape.dims[1] / 16 != output_shape_1.dims[1] || input_shape.dims[2] / 16 != output_shape_1.dims[2]) {
        return false;
    }

    return true;
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
        return postprocessF32();
        default:
        return MA_ENOTSUP;
    }

}

ma_err_t NvidiaDet::postprocessF32() {

    results_.clear();

    // get output
    const auto out0 = this->p_engine_->getOutput(0);
    const auto out1 = this->p_engine_->getOutput(1);

    const auto* data0 = out0.data.f32;
    const auto* data1 = out1.data.f32;

    const auto __output_shape0 = this->p_engine_->getOutputShape(0);
    const auto __output_shape1 = this->p_engine_->getOutputShape(1);

    const auto* bboxs = __output_shape0.dims[3] > __output_shape1.dims[3] ? data0 : data1;
    const auto* conf  = __output_shape0.dims[3] > __output_shape1.dims[3] ? data1 : data0;

    this->conf_shape_   = __output_shape0.dims[3] > __output_shape1.dims[3] ? __output_shape1 : __output_shape0;
    this->bboxes_shape_ = __output_shape0.dims[3] > __output_shape1.dims[3] ? __output_shape0 : __output_shape1;

    const auto H          = this->conf_shape_.dims[1];
    const auto W          = this->conf_shape_.dims[2];
    const auto BboxsCount = this->conf_shape_.dims[3];
    const auto C          = BboxsCount * 4;

    for (int h = 0; h < H; h++) {
        for (int w = 0; w < W; w++) {
            for (int j = 0; j < BboxsCount; j++) {
                if (conf[h * (W * BboxsCount) + w * BboxsCount + j] > 0.2) {
                    ma_bbox_t box;

                    box.x = max(w * this->stride_ + this->offset_ - bboxs[h * (W * C) + w * C + j * 4] * this->scale_, 0) *
                            this->_w_scale;
                    box.y = max(h * this->stride_ + this->offset_ - bboxs[h * (W * C) + w * C + j * 4 + 1] * this->scale_, 0) *
                            this->_h_scale;
                    box.w = (min(w * this->stride_ + this->offset_ + bboxs[h * (W * C) + w * C + j * 4 + 2] * this->scale_,
                                 W * this->stride_)) *
                              this->_w_scale -
                            box.x - 1;
                    box.h = (min(h * this->stride_ + this->offset_ + bboxs[h * (W * C) + w * C + j * 4 + 3] * this->scale_,
                                 H * this->stride_)) *
                              this->_h_scale -
                            box.y - 1;
                    box.x = box.x + box.w / 2;
                    box.y = box.y + box.h / 2;

                    box.score  = conf[h * (W * BboxsCount) + w * BboxsCount + j] * 2.0;
                    box.target = j;

                    results_.push_back(std::move(box));
                }
            }
        }
    }

    ma::utils::nms(results_, threshold_nms_, threshold_score_, false, true);

    results_.shrink_to_fit();

    return EL_OK;
}

}  // namespace ma::model
