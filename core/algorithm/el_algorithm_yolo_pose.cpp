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
#include <numeric>
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
      _last_input_width(0),
      _last_input_height(0),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(score_threshold),
      _iou_threshold(iou_threshold) {
    init();
}

AlgorithmYOLOPOSE::AlgorithmYOLOPOSE(EngineType* engine, const ConfigType& config)
    : Algorithm(engine, config.info),
      _last_input_width(0),
      _last_input_height(0),
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

static inline float sigmoid(float x) { return 1.f / (1.f + std::exp(-x)); }

static inline void softmax(float* data, size_t size) {
    float sum{0.f};
    for (size_t i = 0; i < size; ++i) {
        data[i] = std::exp(data[i]);
        sum += data[i];
    }
    for (size_t i = 0; i < size; ++i) {
        data[i] /= sum;
    }
}

static inline float dequant_value_i(size_t idx, const int8_t* output_array, int32_t zero_point, float scale) {
    return static_cast<float>(output_array[idx] - zero_point) * scale;
}

static decltype(auto) generate_anchor_strides(size_t input_size, std::vector<size_t> strides = {8, 16, 32}) {
    std::vector<anchor_stride_t> anchor_strides(strides.size());
    size_t                       nth_anchor = 0;
    for (size_t i = 0; i < strides.size(); ++i) {
        size_t stride     = strides[i];
        size_t split      = input_size / stride;
        size_t size       = split * split;
        anchor_strides[i] = {stride, split, size, nth_anchor};
        nth_anchor += size;
    }
    return anchor_strides;
}

static decltype(auto) generate_anchor_matrix(const std::vector<anchor_stride_t>& anchor_strides,
                                             float                               shift_right = 1.f,
                                             float                               shift_down  = 1.f) {
    const auto                            anchor_matrix_size = anchor_strides.size();
    std::vector<std::vector<pt_t<float>>> anchor_matrix(anchor_matrix_size);
    const float                           shift_right_init = shift_right * 0.5f;
    const float                           shift_down_init  = shift_down * 0.5f;

    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        const auto& anchor_stride   = anchor_strides[i];
        const auto  split           = anchor_stride.split;
        const auto  size            = anchor_stride.size;
        auto&       anchor_matrix_i = anchor_matrix[i];

        anchor_matrix[i].resize(size);

        for (size_t j = 0; j < size; ++j) {
            float x            = static_cast<float>(j % split) * shift_right + shift_right_init;
            float y            = static_cast<float>(j / split) * shift_down + shift_down_init;
            anchor_matrix_i[j] = {x, y};
        }
    }

    return anchor_matrix;
}

static inline float compute_iou(const types::anchor_bbox_t& l, const types::anchor_bbox_t& r, float epsilon = 1e-3) {
    float x1         = std::max(l.x1, r.x1);
    float y1         = std::max(l.y1, r.y1);
    float x2         = std::min(l.x2, r.x2);
    float y2         = std::min(l.y2, r.y2);
    float w          = std::max(0.0f, x2 - x1);
    float h          = std::max(0.0f, y2 - y1);
    float inter      = w * h;
    float l_area     = (l.x2 - l.x1) * (l.y2 - l.y1);
    float r_area     = (r.x2 - r.x1) * (r.y2 - r.y1);
    float union_area = l_area + r_area - inter;
    if (union_area < epsilon) {
        return 0.0f;
    }
    return inter / union_area;
}

static inline void anchor_nms(std::forward_list<types::anchor_bbox_t>& bboxes,
                              float                                    nms_iou_thresh,
                              float                                    nms_score_thresh,
                              bool                                     soft_nms,
                              float                                    epsilon = 1e-3) {
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

    auto anchor_strides_1 = utils::generate_anchor_strides(std::min(input_shape.dims[1], input_shape.dims[2]));
    auto anchor_strides_2 = anchor_strides_1;
    auto sum =
      std::accumulate(anchor_strides_1.begin(), anchor_strides_1.end(), 0u, [](auto sum, const auto& anchor_stride) {
          return sum + anchor_stride.size;
      });

    for (size_t i = 0; i < _outputs; ++i) {
        const auto output_shape{engine->get_output_shape(i)};
        if (output_shape.size != 3 || output_shape.dims[0] != 1) return false;

        switch (output_shape.dims[2]) {
        case 1: {
            auto it = std::find_if(
              anchor_strides_1.begin(), anchor_strides_1.end(), [&output_shape](const anchor_stride_t& anchor_stride) {
                  return static_cast<int>(anchor_stride.size) == output_shape.dims[1];
              });
            if (it == anchor_strides_1.end())
                return false;
            else
                anchor_strides_1.erase(it);
        } break;
        case 64: {
            auto it = std::find_if(
              anchor_strides_2.begin(), anchor_strides_2.end(), [&output_shape](const anchor_stride_t& anchor_stride) {
                  return static_cast<int>(anchor_stride.size) == output_shape.dims[1];
              });
            if (it == anchor_strides_2.end())
                return false;
            else
                anchor_strides_2.erase(it);
        } break;
        default:
            if (output_shape.dims[2] % 3 != 0) return false;
            if (output_shape.dims[1] != static_cast<int>(sum)) return false;
        }
    }

    if (anchor_strides_1.size() || anchor_strides_2.size()) return false;

    return true;
}

el_err_code_t AlgorithmYOLOPOSE::run(ImageType* input) {
    if (_last_input_width != input->width || _last_input_height != input->height) {
        _last_input_width  = input->width;
        _last_input_height = input->height;

        _w_scale = static_cast<float>(input->width) / static_cast<float>(_input_img.width);
        _h_scale = static_cast<float>(input->height) / static_cast<float>(_input_img.height);

        const auto size = std::min(_anchor_strides.size(), _scaled_strides.size());
        for (size_t i = 0; i < size; ++i) {
            auto stride        = static_cast<float>(_anchor_strides[i].stride);
            _scaled_strides[i] = std::make_pair(stride * _w_scale, stride * _h_scale);
        }
    }

    // TODO: image type conversion before underlying_run, because underlying_run doing a type erasure
    return underlying_run(input);
};

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

    // inputs shape
    const auto width{this->__input_shape.dims[1]};
    const auto height{this->__input_shape.dims[2]};

    // construct stride matrix and anchor matrix
    // TODO: support generate anchor with non-square input
    _anchor_strides = utils::generate_anchor_strides(std::min(width, height));
    _anchor_matrix  = utils::generate_anchor_matrix(_anchor_strides);

    for (size_t i = 0; i < _outputs; ++i) {
        _output_shapes[i]       = this->__p_engine->get_output_shape(i);
        _output_quant_params[i] = this->__p_engine->get_output_quant_param(i);
    }

    _scaled_strides.reserve(_anchor_strides.size());
    for (const auto& anchor_stride : _anchor_strides) {
        _scaled_strides.emplace_back(std::make_pair(static_cast<float>(anchor_stride.stride) * _w_scale,
                                                    static_cast<float>(anchor_stride.stride) * _h_scale));
    }

    for (size_t i = 0; i < _outputs; ++i) {
        // assuimg all outputs has 3 dims and the first dim is 1 (actual validation is done in is_model_valid)
        auto dim_1 = _output_shapes[i].dims[1];
        auto dim_2 = _output_shapes[i].dims[2];

        switch (dim_2) {
        case 1:
            for (size_t j = 0; j < _anchor_variants; ++j) {
                if (dim_1 == static_cast<int>(_anchor_strides[j].size)) {
                    _output_scores_ids[j] = i;
                    break;
                }
            }
            break;
        case 64:
            for (size_t j = 0; j < _anchor_variants; ++j) {
                if (dim_1 == static_cast<int>(_anchor_strides[j].size)) {
                    _output_bboxes_ids[j] = i;
                    break;
                }
            }
            break;
        default:
            if (dim_2 % 3 == 0) {
                _output_keypoints_id = i;
            }
        }
    }

    // check if all outputs ids is unique and less than outputs (redandant)
    using CheckType       = uint8_t;
    size_t    check_bytes = sizeof(CheckType) * 8u;
    CheckType check       = 1 << (_output_keypoints_id % check_bytes);
    for (size_t i = 0; i < _anchor_variants; ++i) {
        CheckType f_s = 1 << (_output_scores_ids[i] % check_bytes);
        CheckType f_b = 1 << (_output_bboxes_ids[i] % check_bytes);
        EL_ASSERT(!(f_s & f_b));
        EL_ASSERT(!(f_s & check));
        EL_ASSERT(!(f_b & check));
        check |= f_s | f_b;
    }
    EL_ASSERT(!(check ^ 0b01111111));
}

el_err_code_t AlgorithmYOLOPOSE::postprocess() {
    _results.clear();

    const int8_t* output_data[_outputs];
    for (size_t i = 0; i < _outputs; ++i) {
        output_data[i] = static_cast<int8_t*>(this->__p_engine->get_output(i));
    }

    // post-process
    const float score_threshold = static_cast<float>(_score_threshold.load()) / 100.f;
    const float iou_threshold   = static_cast<float>(_iou_threshold.load()) / 100.f;

    std::forward_list<types::anchor_bbox_t> anchor_bboxes;

    const auto anchor_matrix_size = _anchor_matrix.size();
    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        const auto  output_scores_id         = _output_scores_ids[i];
        const auto* output_scores            = output_data[output_scores_id];
        const auto  output_scores_quant_parm = _output_quant_params[output_scores_id];

        const auto  output_bboxes_id           = _output_bboxes_ids[i];
        const auto* output_bboxes              = output_data[output_bboxes_id];
        const auto  output_bboxes_shape_dims_2 = _output_shapes[output_bboxes_id].dims[2];
        const auto  output_bboxes_quant_parm   = _output_quant_params[output_bboxes_id];

        const auto  stride  = _scaled_strides[i];
        const float scale_w = stride.first;
        const float scale_h = stride.second;

        const auto& anchor_array      = _anchor_matrix[i];
        const auto  anchor_array_size = anchor_array.size();

        for (size_t j = 0; j < anchor_array_size; ++j) {
            float score = utils::sigmoid(utils::dequant_value_i(
              j, output_scores, output_scores_quant_parm.zero_point, output_scores_quant_parm.scale));

            if (score < score_threshold) continue;

            // DFL
            float dist[4];
            float matrix[16];

            const auto pre = j * output_bboxes_shape_dims_2;
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

            float x1 = (anchor.x - dist[0]) * scale_w;
            float y1 = (anchor.y - dist[1]) * scale_h;
            float x2 = (anchor.x + dist[2]) * scale_w;
            float y2 = (anchor.y + dist[3]) * scale_h;

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

    if (anchor_bboxes.empty()) return EL_OK;

    utils::anchor_nms(anchor_bboxes, iou_threshold, score_threshold, false);

    const auto*  output_keypoints            = output_data[_output_keypoints_id];
    const auto   output_keypoints_dims_2     = _output_shapes[_output_keypoints_id].dims[2];
    const auto   output_keypoints_quant_parm = _output_quant_params[_output_keypoints_id];
    const size_t keypoint_nums               = output_keypoints_dims_2 / 3;

    std::vector<types::pt3_t<float>> n_keypoint(keypoint_nums);

    // extract keypoints from outputs and store all results
    for (const auto& anchor_bbox : anchor_bboxes) {
        const auto pre =
          (_anchor_strides[anchor_bbox.anchor_class].start + anchor_bbox.anchor_index) * output_keypoints_dims_2;

        auto       anchor = _anchor_matrix[anchor_bbox.anchor_class][anchor_bbox.anchor_index];
        const auto stride = _scaled_strides[anchor_bbox.anchor_class];

        anchor.x -= 0.5f;
        anchor.y -= 0.5f;
        const float scale_w = stride.first;
        const float scale_h = stride.second;

        for (size_t i = 0; i < keypoint_nums; ++i) {
            const auto offset = pre + i * 3;

            float x = utils::dequant_value_i(
              offset, output_keypoints, output_keypoints_quant_parm.zero_point, output_keypoints_quant_parm.scale);
            float y = utils::dequant_value_i(
              offset + 1, output_keypoints, output_keypoints_quant_parm.zero_point, output_keypoints_quant_parm.scale);
            float z = utils::dequant_value_i(
              offset + 2, output_keypoints, output_keypoints_quant_parm.zero_point, output_keypoints_quant_parm.scale);

            x = x * 2.f + anchor.x;
            y = y * 2.f + anchor.y;
            z = utils::sigmoid(z);

            n_keypoint[i] = {x, y, z};
        }

        // convert coordinates and rescale bbox
        float cx = (anchor_bbox.x1 + anchor_bbox.x2) * 0.5f;
        float cy = (anchor_bbox.y1 + anchor_bbox.y2) * 0.5f;
        float w  = (anchor_bbox.x2 - anchor_bbox.x1);
        float h  = (anchor_bbox.y2 - anchor_bbox.y1);
        float s  = anchor_bbox.score * 100.f;

        KeyPointType keypoint;
        keypoint.box = {
          .x      = static_cast<decltype(KeyPointType::box.x)>(std::round(cx)),
          .y      = static_cast<decltype(KeyPointType::box.y)>(std::round(cy)),
          .w      = static_cast<decltype(KeyPointType::box.w)>(std::round(w)),
          .h      = static_cast<decltype(KeyPointType::box.h)>(std::round(h)),
          .score  = static_cast<decltype(KeyPointType::box.score)>(std::round(s)),
          .target = static_cast<decltype(KeyPointType::box.target)>(0),
        };
        keypoint.pts.reserve(keypoint_nums);
        size_t target = 0;
        for (const auto& kp : n_keypoint) {
            float x = kp.x * scale_w;
            float y = kp.y * scale_h;
            float z = kp.z * 100.f;
            keypoint.pts.emplace_back(el_point_t{
              .x      = static_cast<decltype(el_point_t::x)>(std::round(x)),
              .y      = static_cast<decltype(el_point_t::y)>(std::round(y)),
              .score  = static_cast<decltype(el_point_t::score)>(std::round(z)),
              .target = static_cast<decltype(el_point_t::target)>(target++),
            });
        }
        keypoint.score  = keypoint.box.score;
        keypoint.target = keypoint.box.target;
        _results.emplace_front(std::move(keypoint));
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
