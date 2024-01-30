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

#include "el_algorithm_yolov8.h"

#include <cmath>
#include <type_traits>

#include "core/el_common.h"
#include "core/el_debug.h"
#include "core/utils/el_cv.h"
#include "core/utils/el_nms.h"

namespace edgelab {

AlgorithmYOLOV8::InfoType AlgorithmYOLOV8::algorithm_info{types::el_algorithm_yolov8_config_t::info};

AlgorithmYOLOV8::AlgorithmYOLOV8(EngineType* engine, ScoreType score_threshold, IoUType iou_threshold)
    : Algorithm(engine, AlgorithmYOLOV8::algorithm_info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(score_threshold),
      _iou_threshold(iou_threshold) {
    init();
}

AlgorithmYOLOV8::AlgorithmYOLOV8(EngineType* engine, const ConfigType& config)
    : Algorithm(engine, config.info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(config.score_threshold),
      _iou_threshold(config.iou_threshold) {
    init();
}

AlgorithmYOLOV8::~AlgorithmYOLOV8() {
    _results.clear();
    this->__p_engine = nullptr;
}

bool AlgorithmYOLOV8::is_model_valid(const EngineType* engine) {
    const auto& input_shape{engine->get_input_shape(0)};
    if (input_shape.size != 4 ||                      // B, W, H, C
        input_shape.dims[0] != 1 ||                   // B = 1
        input_shape.dims[1] ^ input_shape.dims[2] ||  // W = H
        input_shape.dims[1] < 32 ||                   // W, H >= 32
        input_shape.dims[1] % 32 ||                   // W or H is multiply of 32
        (input_shape.dims[3] != 3 &&                  // C = RGB or Gray
         input_shape.dims[3] != 1))
        return false;

    auto ibox_len{[&]() {
        auto r{static_cast<uint16_t>(input_shape.dims[1])};
        auto s{r >> 5};  // r / 32
        auto m{r >> 4};  // r / 16
        auto l{r >> 3};  // r / 8
        return (s * s + m * m + l * l);
    }()};

    const auto& output_shape{engine->get_output_shape(0)};
    if (output_shape.size != 3 ||            // B, IB, BC...
        output_shape.dims[0] != 1 ||         // B = 1
        output_shape.dims[2] != ibox_len ||  // IB is based on input shape
        output_shape.dims[1] < 5 ||          // 6 <= BC - 5[XYWH] <= 80 (could be larger than 80)
        output_shape.dims[1] > 84)
        return false;

    return true;
}

inline void AlgorithmYOLOV8::init() {
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

el_err_code_t AlgorithmYOLOV8::run(ImageType* input) {
    _w_scale = static_cast<float>(input->width) / static_cast<float>(_input_img.width);
    _h_scale = static_cast<float>(input->height) / static_cast<float>(_input_img.height);

    // TODO: image type conversion before underlying_run, because underlying_run doing a type erasure
    return underlying_run(input);
};

el_err_code_t AlgorithmYOLOV8::preprocess() {
    auto* i_img{static_cast<ImageType*>(this->__p_input)};

    // convert image
    el_img_convert(i_img, &_input_img);

    auto size{_input_img.size};
    for (decltype(ImageType::size) i{0}; i < size; ++i) {
        _input_img.data[i] -= 128;
    }

    return EL_OK;
}

el_err_code_t AlgorithmYOLOV8::postprocess() {
    _results.clear();

    // get output
    auto* data{static_cast<int8_t*>(this->__p_engine->get_output(0))};

    auto width{this->__input_shape.dims[1]};
    auto height{this->__input_shape.dims[2]};

    float   scale{this->__output_quant.scale};
    bool    rescale{scale < 0.1f ? true : false};
    int32_t zero_point{this->__output_quant.zero_point};

    auto num_record{this->__output_shape.dims[2]};
    auto num_element{this->__output_shape.dims[1]};
    auto num_class{static_cast<uint8_t>(num_element - 4)};

    ScoreType score_threshold{get_score_threshold()};
    IoUType   iou_threshold{get_iou_threshold()};

    // parse output
    for (decltype(num_record) idx{0}; idx < num_record; ++idx) {
        uint16_t target = 0;
        // get box target
        int8_t max{INT8_MIN};
        for (decltype(num_class) t{0}; t < num_class; ++t) {
            auto n = data[idx + (t + INDEX_T) * num_record];
            if (max < n) {
                max    = n;
                target = t;
            }
        }
        auto score{static_cast<decltype(scale)>(max - zero_point) * scale};
        score = rescale ? score * 100.f : score;
        if (score > score_threshold) {
            BoxType box{
              .x      = 0,
              .y      = 0,
              .w      = 0,
              .h      = 0,
              .score  = static_cast<decltype(BoxType::score)>(std::round(score)),
              .target = static_cast<decltype(BoxType::target)>(target),
            };

            // get box position, int8_t - int32_t (narrowing)
            auto x{((data[idx + INDEX_X * num_record] - zero_point) * scale)};
            auto y{((data[idx + INDEX_Y * num_record] - zero_point) * scale)};
            auto w{((data[idx + INDEX_W * num_record] - zero_point) * scale)};
            auto h{((data[idx + INDEX_H * num_record] - zero_point) * scale)};

            if (rescale) {
                x = x * width;
                y = y * height;
                w = w * width;
                h = h * height;
            }

            box.x = EL_CLIP(x, 0, width) * _w_scale;
            box.y = EL_CLIP(y, 0, height) * _h_scale;
            box.w = EL_CLIP(w, 0, width) * _w_scale;
            box.h = EL_CLIP(h, 0, height) * _h_scale;

            _results.emplace_front(std::move(box));
        }
    }
    el_nms(_results, iou_threshold, score_threshold, false, true);

    _results.sort([](const BoxType& a, const BoxType& b) { return a.x < b.x; });

    return EL_OK;
}

const std::forward_list<AlgorithmYOLOV8::BoxType>& AlgorithmYOLOV8::get_results() const { return _results; }

void AlgorithmYOLOV8::set_score_threshold(ScoreType threshold) { _score_threshold.store(threshold); }

AlgorithmYOLOV8::ScoreType AlgorithmYOLOV8::get_score_threshold() const { return _score_threshold.load(); }

void AlgorithmYOLOV8::set_iou_threshold(IoUType threshold) { _iou_threshold.store(threshold); }

AlgorithmYOLOV8::IoUType AlgorithmYOLOV8::get_iou_threshold() const { return _iou_threshold.load(); }

void AlgorithmYOLOV8::set_algorithm_config(const ConfigType& config) {
    set_score_threshold(config.score_threshold);
    set_iou_threshold(config.iou_threshold);
}

AlgorithmYOLOV8::ConfigType AlgorithmYOLOV8::get_algorithm_config() const {
    ConfigType config;
    config.score_threshold = get_score_threshold();
    config.iou_threshold   = get_iou_threshold();
    return config;
}

}  // namespace edgelab
