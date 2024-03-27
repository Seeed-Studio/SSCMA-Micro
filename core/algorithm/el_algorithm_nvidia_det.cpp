/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Seeed Technology Co.,Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "el_algorithm_nvidia_det.h"

#include <cmath>
#include <type_traits>

#include "core/el_common.h"
#include "core/el_debug.h"
#include "core/utils/el_cv.h"
#include "core/utils/el_nms.h"

namespace edgelab {

AlgorithmNvidiaDet::InfoType AlgorithmNvidiaDet::algorithm_info{types::el_algorithm_nvidia_det_config_t::info};

AlgorithmNvidiaDet::AlgorithmNvidiaDet(EngineType* engine, ScoreType score_threshold, IoUType iou_threshold)
    : Algorithm(engine, AlgorithmNvidiaDet::algorithm_info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(score_threshold),
      _iou_threshold(iou_threshold) {
    init();
}

AlgorithmNvidiaDet::AlgorithmNvidiaDet(EngineType* engine, const ConfigType& config)
    : Algorithm(engine, config.info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(config.score_threshold),
      _iou_threshold(config.iou_threshold) {
    init();
}

AlgorithmNvidiaDet::~AlgorithmNvidiaDet() {
    _results.clear();
    this->__p_engine = nullptr;
}

bool AlgorithmNvidiaDet::is_model_valid(const EngineType* engine) {
    const auto& input_shape{engine->get_input_shape(0)};
    if (input_shape.size != 4 ||                      // B, W, H, C
        input_shape.dims[0] != 1 ||                   // B = 1
        input_shape.dims[1] ^ input_shape.dims[2] ||  // W = H
        input_shape.dims[1] < 32 ||                   // W, H >= 32
        input_shape.dims[1] % 16 ||                   // W or H is multiply of 32
        (input_shape.dims[3] != 3 &&                  // C = RGB or Gray
         input_shape.dims[3] != 1))
        return false;

    auto ibox_len{[&]() {
        auto r{static_cast<uint16_t>(input_shape.dims[1])};
        auto m{r >> 4};  // r / 16
        return (m * m);
    }()};

    return true;
}

inline void AlgorithmNvidiaDet::init() {
    EL_ASSERT(is_model_valid(this->__p_engine));
    EL_ASSERT(_score_threshold.is_lock_free());
    EL_ASSERT(_iou_threshold.is_lock_free());

    _input_img.data   = static_cast<decltype(ImageType::data)>(this->__p_engine->get_input(0));
    _input_img.width  = static_cast<decltype(ImageType::width)>(this->__input_shape.dims[1]),
    _input_img.height = static_cast<decltype(ImageType::height)>(this->__input_shape.dims[2]),
    _input_img.size =
      static_cast<decltype(ImageType::size)>(_input_img.width * _input_img.height * this->__input_shape.dims[3]);
    _input_img.format = EL_PIXEL_FORMAT_UNKNOWN;
    _input_img.rotate = EL_PIXEL_ROTATE_0;
    if (this->__input_shape.dims[3] == 3) {
        _input_img.format = EL_PIXEL_FORMAT_RGB888;
    } else if (this->__input_shape.dims[3] == 1) {
        _input_img.format = EL_PIXEL_FORMAT_GRAYSCALE;
    }
    EL_ASSERT(_input_img.format != EL_PIXEL_FORMAT_UNKNOWN);
    EL_ASSERT(_input_img.rotate != EL_PIXEL_ROTATE_UNKNOWN);
}

el_err_code_t AlgorithmNvidiaDet::run(ImageType* input) {
    _w_scale = static_cast<float>(input->width) / static_cast<float>(_input_img.width);
    _h_scale = static_cast<float>(input->height) / static_cast<float>(_input_img.height);

    // TODO: image type conversion before underlying_run, because underlying_run doing a type erasure
    return underlying_run(input);
};

el_err_code_t AlgorithmNvidiaDet::preprocess() {
    auto* i_img{static_cast<ImageType*>(this->__p_input)};

    // convert image
    el_img_convert(i_img, &_input_img);

    auto size{_input_img.size};
    for (decltype(ImageType::size) i{0}; i < size; ++i) {
        _input_img.data[i] -= 128;
    }

    return EL_OK;
}

float min(float a, float b) {
    if (a < b)
        return a;
    else
        return b;
}

float max(float a, float b) {
    if (a > b)
        return a;
    else
        return b;
}

el_err_code_t AlgorithmNvidiaDet::postprocess() {
    _results.clear();
    el_shape_t __output_shape0;
    el_shape_t __output_shape1;
    // get output
    auto* data0{static_cast<_Float32*>(this->__p_engine->get_output(0))};
    auto* data1{static_cast<_Float32*>(this->__p_engine->get_output(1))};
    __output_shape0 = this->__p_engine->get_output_shape(0);
    __output_shape1 = this->__p_engine->get_output_shape(1);

    auto* bboxs = __output_shape0.dims[3] > __output_shape1.dims[3] ? data0 : data1;
    auto* conf  = __output_shape0.dims[3] > __output_shape1.dims[3] ? data1 : data0;

    this->_conf_shape   = __output_shape0.dims[3] > __output_shape1.dims[3] ? __output_shape1 : __output_shape0;
    this->_bboxes_shape = __output_shape0.dims[3] > __output_shape1.dims[3] ? __output_shape0 : __output_shape1;

    auto B          = this->_conf_shape.dims[0];
    auto H          = this->_conf_shape.dims[1];
    auto W          = this->_conf_shape.dims[2];
    auto BboxsCount = this->_conf_shape.dims[3];
    auto C          = BboxsCount * 4;

    for (int h = 0; h < H; h++) {
        for (int w = 0; w < W; w++) {
            for (int j = 0; j < BboxsCount; j++) {
                if (conf[h * (W * BboxsCount) + w * BboxsCount + j] > 0.2) {
                    BoxType box{
                      .x      = 0,
                      .y      = 0,
                      .w      = 0,
                      .h      = 0,
                      .score  = 0,
                      .target = 0,
                    };
                    box.x = max(w * this->stride + this->offset - bboxs[h * (W * C) + w * C + j * 4] * this->scale, 0) *
                            this->_w_scale;
                    box.y =
                      max(h * this->stride + this->offset - bboxs[h * (W * C) + w * C + j * 4 + 1] * this->scale, 0) *
                      this->_h_scale;
                    box.w = (min(w * this->stride + this->offset + bboxs[h * (W * C) + w * C + j * 4 + 2] * this->scale,
                                 W * this->stride)) *
                              this->_w_scale -
                            box.x - 1;
                    box.h = (min(h * this->stride + this->offset + bboxs[h * (W * C) + w * C + j * 4 + 3] * this->scale,
                                 H * this->stride)) *
                              this->_h_scale -
                            box.y - 1;
                    box.x = box.x + box.w / 2;
                    box.y = box.y + box.h / 2;

                    box.score  = (int)(conf[h * (W * BboxsCount) + w * BboxsCount + j] * 200);
                    box.target = j;
                    _results.emplace_front(std::move(box));
                }
            }
        }
    }

    ScoreType score_threshold{get_score_threshold()};
    IoUType   iou_threshold{get_iou_threshold()};

    el_nms(_results, iou_threshold, score_threshold, false, true);

    // _results.sort([](const BoxType& a, const BoxType& b) { return a.x < b.x; });

    return EL_OK;
}

const std::forward_list<AlgorithmNvidiaDet::BoxType>& AlgorithmNvidiaDet::get_results() const { return _results; }

void AlgorithmNvidiaDet::set_score_threshold(ScoreType threshold) { _score_threshold.store(threshold); }

AlgorithmNvidiaDet::ScoreType AlgorithmNvidiaDet::get_score_threshold() const { return _score_threshold.load(); }

void AlgorithmNvidiaDet::set_iou_threshold(IoUType threshold) { _iou_threshold.store(threshold); }

AlgorithmNvidiaDet::IoUType AlgorithmNvidiaDet::get_iou_threshold() const { return _iou_threshold.load(); }

void AlgorithmNvidiaDet::set_algorithm_config(const ConfigType& config) {
    set_score_threshold(config.score_threshold);
    set_iou_threshold(config.iou_threshold);
}

AlgorithmNvidiaDet::ConfigType AlgorithmNvidiaDet::get_algorithm_config() const {
    ConfigType config;
    config.score_threshold = get_score_threshold();
    config.iou_threshold   = get_iou_threshold();
    return config;
}

}  // namespace edgelab
