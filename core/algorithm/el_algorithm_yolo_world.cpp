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

#include "el_algorithm_yolo_world.h"

#include <cmath>
#include <type_traits>

#include "core/el_common.h"
#include "core/el_debug.h"
#include "core/utils/el_cv.h"
#include "core/utils/el_nms.h"

namespace edgelab {

AlgorithmYOLOWorld::InfoType AlgorithmYOLOWorld::algorithm_info{types::el_algorithm_yolo_world_config_t::info};

AlgorithmYOLOWorld::AlgorithmYOLOWorld(EngineType* engine, ScoreType score_threshold, IoUType iou_threshold)
    : Algorithm(engine, AlgorithmYOLOWorld::algorithm_info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(score_threshold),
      _iou_threshold(iou_threshold) {
    init();
}

AlgorithmYOLOWorld::AlgorithmYOLOWorld(EngineType* engine, const ConfigType& config)
    : Algorithm(engine, config.info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(config.score_threshold),
      _iou_threshold(config.iou_threshold) {
    init();
}

AlgorithmYOLOWorld::~AlgorithmYOLOWorld() {
    _results.clear();
    this->__p_engine = nullptr;
}

bool AlgorithmYOLOWorld::is_model_valid(const EngineType* engine) {
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

    const auto& output_shape_0{engine->get_output_shape(0)};
    if (output_shape_0.size != 3 ||            // B, IB, BC...
        output_shape_0.dims[0] != 1 ||         // B = 1
        output_shape_0.dims[1] != ibox_len ||  // IB is based on input shape
        output_shape_0.dims[2] != 4)
        return false;

    const auto& output_shape_1{engine->get_output_shape(1)};
    if (output_shape_1.size != 3 ||            // B, IB, BC...
        output_shape_1.dims[0] != 1 ||         // B = 1
        output_shape_1.dims[1] != ibox_len ||  // IB is based on input shape
        output_shape_1.dims[2] < 1 ||          // 0 < T <= 80 (could be larger than 80)
        output_shape_1.dims[2] > 80)
        return false;

    return true;
}

inline void AlgorithmYOLOWorld::init() {
    EL_ASSERT(is_model_valid(this->__p_engine));
    EL_ASSERT(_score_threshold.is_lock_free());
    EL_ASSERT(_iou_threshold.is_lock_free());

    for (size_t i = 0; i < _outputs; ++i) {
        _output_shapes[i]       = this->__p_engine->get_output_shape(i);
        _output_quant_params[i] = this->__p_engine->get_output_quant_param(i);
    }

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

el_err_code_t AlgorithmYOLOWorld::run(ImageType* input) {
    _w_scale = static_cast<float>(input->width) / static_cast<float>(_input_img.width);
    _h_scale = static_cast<float>(input->height) / static_cast<float>(_input_img.height);

    // TODO: image type conversion before underlying_run, because underlying_run doing a type erasure
    return underlying_run(input);
};

el_err_code_t AlgorithmYOLOWorld::preprocess() {
    auto* i_img{static_cast<ImageType*>(this->__p_input)};

    // convert image
    el_img_convert(i_img, &_input_img);

    auto size{_input_img.size};
    for (decltype(ImageType::size) i{0}; i < size; ++i) {
        _input_img.data[i] -= 128;
    }

    return EL_OK;
}

el_err_code_t AlgorithmYOLOWorld::postprocess() {
    _results.clear();

    // get outputs
    auto* data_bboxes{static_cast<int8_t*>(this->__p_engine->get_output(0))};
    auto* data_scores{static_cast<int8_t*>(this->__p_engine->get_output(1))};

    auto width{this->__input_shape.dims[1]};
    auto height{this->__input_shape.dims[2]};

    float scale_scores{_output_quant_params[1].scale};
    scale_scores = scale_scores < 0.1f ? scale_scores * 100.f : scale_scores;  // rescale
    int32_t zero_point_scores{_output_quant_params[1].zero_point};

    float   scale_bboxes{_output_quant_params[0].scale};
    int32_t zero_point_bboxes{_output_quant_params[0].zero_point};

    auto num_bboxes{this->__output_shape.dims[1]};
    auto num_element{this->__output_shape.dims[2]};
    auto num_classes{static_cast<uint8_t>(_output_shapes[1].dims[2])};

    ScoreType score_threshold{get_score_threshold()};
    IoUType   iou_threshold{get_iou_threshold()};

    // parse output
    for (size_t target{0}; target < num_classes; ++target) {
        size_t idx     = target * num_bboxes;
        size_t idx_end = idx + num_bboxes;

        for (size_t bbox_i{idx}; bbox_i < idx_end; ++bbox_i) {
            uint8_t bbox_i_score =
              static_cast<decltype(scale_scores)>(data_scores[bbox_i] - zero_point_scores) * scale_scores;
            if (bbox_i_score < score_threshold) {
                continue;
            }

            BoxType box{
              .x      = 0,
              .y      = 0,
              .w      = 0,
              .h      = 0,
              .score  = static_cast<decltype(BoxType::score)>(bbox_i_score),
              .target = static_cast<decltype(BoxType::target)>(target),
            };

            auto tl_x{((data_bboxes[idx + INDEX_TL_X] - zero_point_bboxes) * scale_bboxes)};
            auto tl_y{((data_bboxes[idx + INDEX_TL_Y] - zero_point_bboxes) * scale_bboxes)};
            auto br_x{((data_bboxes[idx + INDEX_BR_X] - zero_point_bboxes) * scale_bboxes)};
            auto br_y{((data_bboxes[idx + INDEX_BR_Y] - zero_point_bboxes) * scale_bboxes)};

            box.w = br_x - tl_x;
            box.h = br_y - tl_y;
            box.x = tl_x + box.w / 2;
            box.y = tl_y + box.h / 2;

            box.x = EL_CLIP(box.x, 0, width) * _w_scale;
            box.y = EL_CLIP(box.y, 0, height) * _h_scale;
            box.w = EL_CLIP(box.w, 0, width) * _w_scale;
            box.h = EL_CLIP(box.h, 0, height) * _h_scale;

            _results.emplace_front(std::move(box));
        }
    }

    el_nms(_results, iou_threshold, score_threshold, false, true);

    _results.sort([](const BoxType& a, const BoxType& b) { return a.x < b.x; });

    return EL_OK;
}

const std::forward_list<AlgorithmYOLOWorld::BoxType>& AlgorithmYOLOWorld::get_results() const { return _results; }

void AlgorithmYOLOWorld::set_score_threshold(ScoreType threshold) { _score_threshold.store(threshold); }

AlgorithmYOLOWorld::ScoreType AlgorithmYOLOWorld::get_score_threshold() const { return _score_threshold.load(); }

void AlgorithmYOLOWorld::set_iou_threshold(IoUType threshold) { _iou_threshold.store(threshold); }

AlgorithmYOLOWorld::IoUType AlgorithmYOLOWorld::get_iou_threshold() const { return _iou_threshold.load(); }

void AlgorithmYOLOWorld::set_algorithm_config(const ConfigType& config) {
    set_score_threshold(config.score_threshold);
    set_iou_threshold(config.iou_threshold);
}

AlgorithmYOLOWorld::ConfigType AlgorithmYOLOWorld::get_algorithm_config() const {
    ConfigType config;
    config.score_threshold = get_score_threshold();
    config.iou_threshold   = get_iou_threshold();
    return config;
}

}  // namespace edgelab
