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

#include <algorithm>
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
      _iou_threshold(iou_threshold),
      _last_input_width(0),
      _last_input_height(0) {
    init();
}

AlgorithmYOLOPOSE::AlgorithmYOLOPOSE(EngineType* engine, const ConfigType& config)
    : Algorithm(engine, config.info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(config.score_threshold),
      _iou_threshold(config.iou_threshold),
      _last_input_width(0),
      _last_input_height(0) {
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

inline float dequant_value_i(size_t idx, const int8_t* output_array, int32_t zero_point, float scale) {
    return static_cast<float>(output_array[idx] - zero_point) * scale;
}

decltype(auto) generate_anchor_strides(size_t input_size, std::vector<size_t> splits = {8, 16, 32}) {
    std::vector<types::anchor_stride_t> anchor_strides(splits.size());
    size_t                              position = 0;
    for (size_t i = 0; i < splits.size(); ++i) {
        size_t stride     = input_size / splits[i];
        size_t size       = stride * stride;
        anchor_strides[i] = {stride, position, size};
        position += size;
    }
    return anchor_strides;
}

decltype(auto) generate_anchor_matrix(const std::vector<types::anchor_stride_t>& anchor_strides,
                                      float                                      shift_right = 1.f,
                                      float                                      shift_down  = 1.f) {
    const auto                                   anchor_matrix_size = anchor_strides.size();
    std::vector<std::vector<types::pt_t<float>>> anchor_matrix(anchor_matrix_size);
    float                                        shift_right_init = shift_right * 0.5f;
    float                                        shift_down_init  = shift_down * 0.5f;

    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        const auto& anchor_stride   = anchor_strides[i];
        const auto  stride          = anchor_stride.stride;
        const auto  size            = anchor_stride.size;
        auto&       anchor_matrix_i = anchor_matrix[i];

        anchor_matrix[i].resize(size);

        for (size_t j = 0; j < size; ++j) {
            float x            = static_cast<float>(j % stride) * shift_right + shift_right_init;
            float y            = static_cast<float>(j / stride) * shift_down + shift_down_init;
            anchor_matrix_i[j] = {x, y};
        }
    }

    return anchor_matrix;
}

float compute_iou(const types::anchor_bbox_t& l, const types::anchor_bbox_t& r) {
    float x1    = std::max(l.x1, r.x1);
    float y1    = std::max(l.y1, r.y1);
    float x2    = std::min(l.x2, r.x2);
    float y2    = std::min(l.y2, r.y2);
    float w     = std::max(0.0f, x2 - x1);
    float h     = std::max(0.0f, y2 - y1);
    float inter = w * h;
    float iou   = inter / ((l.x2 - l.x1) * (l.y2 - l.y1) + (r.x2 - r.x1) * (r.y2 - r.y1) - inter);
    return iou;
}

void anchor_nms(std::forward_list<types::anchor_bbox_t>& bboxes,
                float                                    nms_iou_thresh,
                float                                    nms_score_thresh,
                bool                                     soft_nms,
                float                                    epsilon = 1e-6) {
    bboxes.sort([](const types::anchor_bbox_t& l, const types::anchor_bbox_t& r) { return l.score > r.score; });

    for (auto it = bboxes.begin(); it != bboxes.end(); it++) {
        if (it->score < epsilon) continue;

        for (auto it2 = std::next(it); it2 != bboxes.end(); it2++) {
            if (it2->score < epsilon) continue;

            auto iou = compute_iou(*it, *it2);
            if (iou > nms_iou_thresh) {
                if (soft_nms) {
                    it2->score = it2->score * (1.f - iou);
                    if (it2->score < nms_score_thresh) it2->score = 0.f;
                } else {
                    it2->score = 0.f;
                }
            }
        }
    }

    bboxes.remove_if([epsilon](const types::anchor_bbox_t& bbox) { return bbox.score < epsilon; });
}

}  // namespace utils

el_err_code_t AlgorithmYOLOPOSE::postprocess() {
    _results.clear();

    // inputs shape
    const auto width{this->__input_shape.dims[1]};
    const auto height{this->__input_shape.dims[2]};

    // construct stride matrix and anchor matrix if input shape changed
    if (width != _last_input_width || height != _last_input_height) {
        _last_input_width  = width;
        _last_input_height = height;

        // TODO: support generate anchor with non-square input
        _anchor_strides = utils::generate_anchor_strides(width);
        _anchor_matrix  = utils::generate_anchor_matrix(_anchor_strides);
    }

    // fetch output details from interpreter
    EL_ASSERT(width == height);
    size_t           input_size = width;
    constexpr size_t outputs    = 7;

    const int8_t*    output_data[outputs];
    el_shape_t       output_shapes[outputs];
    el_quant_param_t output_quant_params[outputs];

    for (size_t i = 0; i < outputs; ++i) {
        output_data[i]         = static_cast<int8_t*>(this->__p_engine->get_output(i));
        output_shapes[i]       = this->__p_engine->get_output_shape(i);
        output_quant_params[i] = this->__p_engine->get_output_quant_param(i);
    }

    // post-process
    const float      score_threshold     = static_cast<float>(_score_threshold.load()) / 100.f;
    const float      iou_threshold       = static_cast<float>(_iou_threshold.load()) / 100.f;
    constexpr size_t output_scores_ids[] = {4, 6, 2};
    constexpr size_t output_bboxes_ids[] = {1, 0, 5};

    std::forward_list<types::anchor_bbox_t> anchor_bboxes;

    const auto anchor_matrix_size = _anchor_matrix.size();
    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        // for each size of bboxes
        const auto  output_scores_id         = output_scores_ids[i];
        const auto* output_scores            = output_data[output_scores_id];
        const auto& output_scores_shape      = output_shapes[output_scores_id];
        const auto  output_scores_quant_parm = output_quant_params[output_scores_id];

        const auto  output_bboxes_id         = output_bboxes_ids[i];
        const auto* output_bboxes            = output_data[output_bboxes_id];
        const auto& output_bboxes_shape      = output_shapes[output_bboxes_id];
        const auto  output_bboxes_quant_parm = output_quant_params[output_bboxes_id];

        const auto& anchor_array      = _anchor_matrix[i];
        const auto  anchor_array_size = anchor_array.size();

        EL_ASSERT(output_scores_shape.dims[1] == anchor_array_size);
        EL_ASSERT(output_bboxes_shape.dims[1] == anchor_array_size);
        EL_ASSERT(output_bboxes_shape.dims[2] == 64);

        for (size_t j = 0; j < anchor_array_size; ++j) {
            float score = utils::sigmoid(utils::dequant_value_i(
              j, output_scores, output_scores_quant_parm.zero_point, output_scores_quant_parm.scale));

            if (score < score_threshold) continue;

            // DFL
            float dist[4];
            float matrix[16];

            const size_t pre = j * output_bboxes_shape.dims[2];
            for (size_t m = 0; m < 4; ++m) {
                const size_t offset = pre + m * 16;
                for (size_t n = 0; n < 16; ++n) {
                    matrix[n] = utils::dequant_value_i(
                      offset + n, output_bboxes, output_bboxes_quant_parm.zero_point, output_bboxes_quant_parm.scale);
                }

                utils::softmax(matrix, 16);

                float res = 0.f;
                for (size_t n = 0; n < 16; ++n) {
                    res += matrix[n] * static_cast<float>(n);
                }
                dist[m] = res;
            }

            const auto anchor = anchor_array[j];

            float x1 = anchor.x - dist[0];
            float y1 = anchor.y - dist[1];
            float x2 = anchor.x + dist[2];
            float y2 = anchor.y + dist[3];

            anchor_bboxes.emplace_front(types::anchor_bbox_t{
              .x1           = x1,
              .y1           = y1,
              .x2           = x2,
              .y2           = y2,
              .score        = score,
              .anchor_class = static_cast<decltype(types::anchor_bbox_t::anchor_class)>(i),
              .anchor_index = static_cast<decltype(types::anchor_bbox_t::anchor_index)>(j),
            });
        }
    }

    utils::anchor_nms(anchor_bboxes, iou_threshold, score_threshold, false);

    const auto*  output_keypoints            = output_data[3];
    const auto&  output_keypoints_shape      = output_shapes[3];
    const auto   output_keypoints_quant_parm = output_quant_params[3];
    const size_t keypoint_nums               = output_keypoints_shape.dims[2] / 3;

    std::vector<types::pt3_t<float>> n_keypoint(keypoint_nums);

    // extract keypoints from outputs and store all results
    size_t target = 0;
    for (const auto& anchor_bbox : anchor_bboxes) {
        const auto pre =
          (_anchor_strides[anchor_bbox.anchor_class].start + anchor_bbox.anchor_index) * output_keypoints_shape.dims[2];

        const auto anchor = _anchor_matrix[anchor_bbox.anchor_class][anchor_bbox.anchor_index];
        const auto stride = _anchor_strides[anchor_bbox.anchor_class].stride;

        float scale_w = stride * _w_scale;
        float scale_h = stride * _h_scale;

        for (size_t i = 0; i < keypoint_nums; ++i) {
            types::pt3_t<float> keypoint;
            const auto          offset = pre + i * 3;

            float x = utils::dequant_value_i(
              offset, output_keypoints, output_keypoints_quant_parm.zero_point, output_keypoints_quant_parm.scale);
            float y = utils::dequant_value_i(
              offset + 1, output_keypoints, output_keypoints_quant_parm.zero_point, output_keypoints_quant_parm.scale);
            float z = utils::dequant_value_i(
              offset + 2, output_keypoints, output_keypoints_quant_parm.zero_point, output_keypoints_quant_parm.scale);

            x = x * 2.f + (anchor.x - 0.5f);
            y = y * 2.f + (anchor.y - 0.5f);
            z = utils::sigmoid(z);

            n_keypoint[i] = {x, y, z};
        }

        // convert coordinates and rescale bbox
        float cx = (anchor_bbox.x1 + anchor_bbox.x2) * 0.5f * scale_w;
        float cy = (anchor_bbox.y1 + anchor_bbox.y2) * 0.5f * scale_h;
        float w  = (anchor_bbox.x2 - anchor_bbox.x1) * scale_w;
        float h  = (anchor_bbox.y2 - anchor_bbox.y1) * scale_h;
        float s  = anchor_bbox.score * 100.f;

        KeyPointType keypoint;
        keypoint.box = {
          .x      = static_cast<decltype(KeyPointType::box.x)>(std::round(cx)),
          .y      = static_cast<decltype(KeyPointType::box.y)>(std::round(cy)),
          .w      = static_cast<decltype(KeyPointType::box.w)>(std::round(w)),
          .h      = static_cast<decltype(KeyPointType::box.h)>(std::round(h)),
          .score  = static_cast<decltype(KeyPointType::box.score)>(std::round(s)),
          .target = static_cast<decltype(KeyPointType::box.target)>(target),
        };
        keypoint.pts.reserve(keypoint_nums);
        for (const auto& kp : n_keypoint) {
            float x = kp.x * scale_w;
            float y = kp.y * scale_h;
            float z = kp.z * 100.f;
            keypoint.pts.emplace_back(el_point_t{
              .x      = static_cast<decltype(el_point_t::x)>(std::round(x)),
              .y      = static_cast<decltype(el_point_t::y)>(std::round(y)),
              .score  = static_cast<decltype(el_point_t::score)>(std::round(z)),
              .target = static_cast<decltype(el_point_t::target)>(target),
            });
        }
        keypoint.score  = keypoint.box.score;
        keypoint.target = keypoint.box.target;
        _results.emplace_front(std::move(keypoint));

        ++target;
    }

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
