/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
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

#include "el_algorithm_yolo_pose.h"

#include <array>
#include <cmath>
#include <type_traits>
#include <vector>

#include "core/el_common.h"
#include "core/el_debug.h"
#include "core/utils/el_cv.h"
#include "core/utils/el_nms.h"

namespace edgelab {

AlgorithmYOLOPOSE::InfoType AlgorithmYOLOPOSE::algorithm_info{types::el_algorithm_yolo_pose_config_t::info};

AlgorithmYOLOPOSE::AlgorithmYOLOPOSE(EngineType* engine, ScoreType score_threshold, IoUType iou_threshold)
    : Algorithm(engine, AlgorithmYOLOPOSE::algorithm_info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(score_threshold),
      _iou_threshold(iou_threshold) {
    init();
}

AlgorithmYOLOPOSE::AlgorithmYOLOPOSE(EngineType* engine, const ConfigType& config)
    : Algorithm(engine, config.info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(config.score_threshold),
      _iou_threshold(config.iou_threshold) {
    init();
}

AlgorithmYOLOPOSE::~AlgorithmYOLOPOSE() {
    _results.clear();
    this->__p_engine = nullptr;
}

bool AlgorithmYOLOPOSE::is_model_valid(const EngineType* engine) {
    const auto& input_shape{engine->get_input_shape(0)};
    if (input_shape.size != 4 ||                      // B, W, H, C
        input_shape.dims[0] != 1 ||                   // B = 1
        input_shape.dims[1] ^ input_shape.dims[2] ||  // W = H
        input_shape.dims[1] < 32 ||                   // W, H >= 32
        input_shape.dims[1] % 32 ||                   // W or H is multiply of 32
        (input_shape.dims[3] != 3 &&                  // C = RGB or Gray
         input_shape.dims[3] != 1))
        return false;

    // TODO: check each output shape of the model
    uint8_t check = 0b00000000;
    for (size_t i = 0; i < 7; ++i) {
        const auto& output_shape{engine->get_output_shape(i)};
        if (output_shape.size != 0) check |= 1 << i;
    }
    if (check ^ 0b01111111) return false;

    return true;
}

inline void AlgorithmYOLOPOSE::init() {
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

el_err_code_t AlgorithmYOLOPOSE::run(ImageType* input) {
    _w_scale = static_cast<float>(input->width) / static_cast<float>(_input_img.width);
    _h_scale = static_cast<float>(input->height) / static_cast<float>(_input_img.height);

    // TODO: image type conversion before underlying_run, because underlying_run doing a type erasure
    return underlying_run(input);
};

el_err_code_t AlgorithmYOLOPOSE::preprocess() {
    auto* i_img{static_cast<ImageType*>(this->__p_input)};

    // convert image
    el_img_convert(i_img, &_input_img);

    auto size{_input_img.size};
    for (decltype(ImageType::size) i{0}; i < size; ++i) {
        _input_img.data[i] -= 128;
    }

    return EL_OK;
}

namespace types {

struct raw_output_t {
    int8_t*          data;
    el_shape_t       shape;
    el_quant_param_t quant_param;
};

}  // namespace types

namespace utils {

inline float sigmoid(float x) { return 1.f / (1.f + std::exp(-x)); }

inline void softmax(float* data, size_t size) {
    float sum{0.f};
    for (size_t i = 0; i < size; ++i) {
        data[i] = std::exp(data[i]);
        sum += data[i];
    }
    for (size_t i = 0; i < size; ++i) {
        data[i] /= sum;
    }
}

inline float dequant_value_i(size_t idx, const types::raw_output_t& output) {
    return (static_cast<float>(output.data[idx]) - static_cast<float>(output.quant_param.zero_point)) *
           output.quant_param.scale;
}

}  // namespace utils

el_err_code_t AlgorithmYOLOPOSE::postprocess() {
    _results.clear();

    // inputs shape
    const auto width{this->__input_shape.dims[1]};
    const auto height{this->__input_shape.dims[2]};

    // fetch output details from interpreter
    EL_ASSERT(width == height);
    size_t           input_size = width;
    constexpr size_t outputs    = 7;

    types::raw_output_t raw_outputs[outputs];
    for (size_t i = 0; i < outputs; ++i) {
        raw_outputs[i].data        = static_cast<int8_t*>(this->__p_engine->get_output(i));
        raw_outputs[i].shape       = this->__p_engine->get_output_shape(i);
        raw_outputs[i].quant_param = this->__p_engine->get_output_quant_param(i);
    }

    // construct stride matrix and anchor matrix
    constexpr size_t splits[]  = {8, 16, 32};
    const size_t     strides[] = {input_size / splits[0], input_size / splits[1], input_size / splits[2]};
    const size_t     anchors[] = {strides[0] * strides[0], strides[1] * strides[1], strides[2] * strides[2]};

    std::vector<std::array<float, 2>> anchor_matrix(anchors[0] + anchors[1] + anchors[2]);

    size_t          stride_start = 0;
    size_t          stride_end   = 0;
    constexpr float anchor_shift = 0.5f;

    for (size_t i = 0; i < 3; ++i) {
        stride_start = stride_end;
        stride_end   = stride_start + anchors[i];

        float      anchor_x = 0.f + anchor_shift;
        float      anchor_y = -1.f + anchor_shift;
        const auto stride   = strides[i];

        for (size_t j = stride_start; j < stride_end; ++j) {
            if (j % stride == 0) {
                anchor_x = 0.f + anchor_shift;
                anchor_y += 1.f;
            }

            // assign anchor location to anchor matrix
            anchor_matrix[j][0] = anchor_x;
            anchor_matrix[j][1] = anchor_y;

            anchor_x += 1.f;
        }
    }

    // post-process
    const float      score_threshold     = static_cast<float>(_score_threshold.load()) / 100.f;
    constexpr size_t output_scores_ids[] = {4, 6, 2};
    constexpr size_t output_bboxes_ids[] = {1, 0, 5};

    std::forward_list<el_box_t> bboxes;

    stride_start = 0;
    stride_end   = 0;
    for (size_t i = 0; i < 3; ++i) {
        stride_start = stride_end;
        stride_end   = stride_start + anchors[i];

        // for each size of bboxes
        const auto& output_scores = raw_outputs[output_scores_ids[i]];
        for (size_t j = stride_start, k = 0; j < stride_end; ++j, ++k) {
            float score = utils::sigmoid(utils::dequant_value_i(k, output_scores));

            if (score < score_threshold) continue;

            // DFL
            float        dist[4];
            float        matrix[16];
            const auto&  output_bboxes = raw_outputs[output_bboxes_ids[i]];
            const size_t pre           = k * output_bboxes.shape.dims[2];
            for (size_t m = 0; m < 4; ++m) {
                const size_t offset = pre + m * 16;
                for (size_t n = 0; n < 16; ++n) {
                    matrix[n] = utils::dequant_value_i(offset + n, output_bboxes);
                }

                utils::softmax(matrix, 16);

                float res = 0.f;
                for (size_t n = 0; n < 16; ++n) {
                    res += matrix[n] * static_cast<float>(n);
                }
                dist[m] = res;
            }

            const float stride = static_cast<float>(strides[i]);
            const auto& anchor = anchor_matrix[j];

            float x1 = anchor[0] - dist[0];
            float y1 = anchor[1] - dist[1];
            float x2 = anchor[0] + dist[2];
            float y2 = anchor[1] + dist[3];

            float cx = x1 + x2;
            float cy = y1 + y2;
            float w  = x2 - x1;
            float h  = y2 - y1;

            float fw = static_cast<float>(width);
            float fh = static_cast<float>(height);

            el_box_t bbox;
            bbox.x      = std::round(EL_CLIP(cx, 0.f, fw) * 0.5f * stride * _w_scale);
            bbox.y      = std::round(EL_CLIP(cy, 0.f, fh) * 0.5f * stride * _h_scale);
            bbox.w      = std::round(EL_CLIP(w, 0.f, fw) * stride * _w_scale);
            bbox.h      = std::round(EL_CLIP(h, 0.f, fh) * stride * _h_scale);
            bbox.score  = std::round(score * 100.f);
            bbox.target = 0;

            bboxes.emplace_front(bbox);
        }
    }

    el_nms(bboxes, _iou_threshold.load(), _score_threshold.load(), false, false);

    std::move(bboxes.begin(), bboxes.end(), std::front_inserter(_results));

    return EL_OK;
}

const std::forward_list<AlgorithmYOLOPOSE::KeyPointType>& AlgorithmYOLOPOSE::get_results() const { return _results; }

void AlgorithmYOLOPOSE::set_score_threshold(ScoreType threshold) { _score_threshold.store(threshold); }

AlgorithmYOLOPOSE::ScoreType AlgorithmYOLOPOSE::get_score_threshold() const { return _score_threshold.load(); }

void AlgorithmYOLOPOSE::set_iou_threshold(IoUType threshold) { _iou_threshold.store(threshold); }

AlgorithmYOLOPOSE::IoUType AlgorithmYOLOPOSE::get_iou_threshold() const { return _iou_threshold.load(); }

void AlgorithmYOLOPOSE::set_algorithm_config(const ConfigType& config) {
    set_score_threshold(config.score_threshold);
    set_iou_threshold(config.iou_threshold);
}

AlgorithmYOLOPOSE::ConfigType AlgorithmYOLOPOSE::get_algorithm_config() const {
    ConfigType config;
    config.score_threshold = get_score_threshold();
    config.iou_threshold   = get_iou_threshold();
    return config;
}

}  // namespace edgelab
