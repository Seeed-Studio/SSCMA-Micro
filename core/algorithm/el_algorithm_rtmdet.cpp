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

#include "el_algorithm_rtmdet.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "core/el_common.h"
#include "core/el_debug.h"
#include "core/utils/el_cv.h"
#include "core/utils/el_nms.h"

namespace edgelab {

AlgorithmRtmdet::InfoType AlgorithmRtmdet::algorithm_info{types::el_algorithm_rtmdet_config_t::info};

AlgorithmRtmdet::AlgorithmRtmdet(EngineType* engine, ScoreType score_threshold, IoUType iou_threshold)
    : Algorithm(engine, AlgorithmRtmdet::algorithm_info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(score_threshold),
      _iou_threshold(iou_threshold) {
    init();
}

AlgorithmRtmdet::AlgorithmRtmdet(EngineType* engine, const ConfigType& config)
    : Algorithm(engine, config.info),
      _w_scale(1.f),
      _h_scale(1.f),
      _score_threshold(config.score_threshold),
      _iou_threshold(config.iou_threshold) {
    init();
}

AlgorithmRtmdet::~AlgorithmRtmdet() {
    _results.clear();
    this->__p_engine = nullptr;
}

namespace utils {

static inline float fast_ln(float x) {
    // Remez approximation of ln(x)
    static_assert(sizeof(unsigned int) == sizeof(float));
    static_assert(sizeof(float) == 4);

    auto bx{*reinterpret_cast<unsigned int*>(&x)};
    auto ex{bx >> 23};
    auto t{static_cast<signed int>(ex) - static_cast<signed int>(127)};
    bx = 1065353216 | (bx & 8388607);
    x  = *reinterpret_cast<float*>(&bx);
    return -1.49278 + (2.11263 + (-0.729104 + 0.10969 * x) * x) * x + 0.6931471806 * t;
}

static inline float fast_exp(float x) {
    // N. Schraudolph, "A Fast, Compact Approximation of the Exponential Function"
    // https://nic.schraudolph.org/pubs/Schraudolph99.pdf
    static_assert(sizeof(float) == 4);

    x = (12102203.1608621 * x) + 1064986823.01029;

    const float c{8388608.f};
    const float d{2139095040.f};

    if (x < c || x > d) x = (x < c) ? 0.0f : d;

    uint32_t n = static_cast<uint32_t>(x);
    x          = *reinterpret_cast<float*>(&n);

    return x;
}

static inline float sigmoid(float x) { return 1.f / (1.f + fast_exp(-x)); }

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

// Unanle to generate firmware if don't use lambda
auto generate_anchor_strides = [](size_t input_size, std::vector<size_t> strides = {8, 16, 32}) {
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
};

// Unanle to generate firmware if don't use lambda
auto generate_anchor_matrix =
  [](const std::vector<anchor_stride_t>& anchor_strides, float shift_right = 1.f, float shift_down = 1.f) {
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
  };

}  // namespace utils

bool AlgorithmRtmdet::is_model_valid(const EngineType* engine) {
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

    // Note: would fail if the model has 4 classes
    for (size_t i = 0; i < _outputs; ++i) {
        const auto output_shape{engine->get_output_shape(i)};
        if (output_shape.size != 3 || output_shape.dims[0] != 1) return false;

        if (output_shape.dims[2] == 4) {
            auto it = std::find_if(
              anchor_strides_2.begin(), anchor_strides_2.end(), [&output_shape](const anchor_stride_t& anchor_stride) {
                  return static_cast<int>(anchor_stride.size) == output_shape.dims[1];
              });
            if (it == anchor_strides_2.end())
                return false;
            else
                anchor_strides_2.erase(it);
        } else {
            auto it = std::find_if(
              anchor_strides_1.begin(), anchor_strides_1.end(), [&output_shape](const anchor_stride_t& anchor_stride) {
                  return static_cast<int>(anchor_stride.size) == output_shape.dims[1];
              });
            if (it == anchor_strides_1.end())
                return false;
            else
                anchor_strides_1.erase(it);
        }
    }

    if (anchor_strides_1.size() || anchor_strides_2.size()) return false;

    return true;
}

inline void AlgorithmRtmdet::init() {
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

    // inputs shape
    const auto width{this->__input_shape.dims[1]};
    const auto height{this->__input_shape.dims[2]};

    // construct stride matrix and anchor matrix
    // TODO: support generate anchor with non-square input
    {
        auto _anchor_strides  = utils::generate_anchor_strides(std::min(width, height));
        auto _anchor_matrix   = utils::generate_anchor_matrix(_anchor_strides);
        this->_anchor_strides = std::move(_anchor_strides);
        this->_anchor_matrix  = std::move(_anchor_matrix);
    }

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

        if (dim_2 == 4) {
            for (size_t j = 0; j < _anchor_variants; ++j) {
                if (dim_1 == static_cast<int>(_anchor_strides[j].size)) {
                    _output_bboxes_ids[j] = i;
                    break;
                }
            }
        } else {
            for (size_t j = 0; j < _anchor_variants; ++j) {
                if (dim_1 == static_cast<int>(_anchor_strides[j].size)) {
                    _output_scores_ids[j] = i;
                    break;
                }
            }
        }
    }

    // check if all outputs ids is unique and less than outputs (redandant)
    using CheckType       = uint8_t;
    size_t    check_bytes = sizeof(CheckType) * 8u;
    CheckType check       = 0;
    for (size_t i = 0; i < _anchor_variants; ++i) {
        CheckType f_s = 1 << (_output_scores_ids[i] % check_bytes);
        CheckType f_b = 1 << (_output_bboxes_ids[i] % check_bytes);
        EL_ASSERT(!(f_s & f_b));
        EL_ASSERT(!(f_s & check));
        EL_ASSERT(!(f_b & check));
        check |= f_s | f_b;
    }
    EL_ASSERT(!(check ^ 0b00111111));
}

el_err_code_t AlgorithmRtmdet::run(ImageType* input) {
    _w_scale = static_cast<float>(input->width) / static_cast<float>(_input_img.width);
    _h_scale = static_cast<float>(input->height) / static_cast<float>(_input_img.height);

    // TODO: image type conversion before underlying_run, because underlying_run doing a type erasure
    return underlying_run(input);
};

el_err_code_t AlgorithmRtmdet::preprocess() {
    auto* i_img{static_cast<ImageType*>(this->__p_input)};

    // convert image
    el_img_convert(i_img, &_input_img);

    auto size{_input_img.size};
    for (decltype(ImageType::size) i{0}; i < size; ++i) {
        _input_img.data[i] -= 128;
    }

    return EL_OK;
}

el_err_code_t AlgorithmRtmdet::postprocess() {
    _results.clear();

    const int8_t* output_data[_outputs];
    for (size_t i = 0; i < _outputs; ++i) {
        output_data[i] = static_cast<int8_t*>(this->__p_engine->get_output(i));
    }

    // post-process
    const uint8_t score_threshold = _score_threshold.load();
    const uint8_t iou_threshold   = _iou_threshold.load();

    const float score_threshold_non_sigmoid =
      -1.f * utils::fast_ln((100.f / static_cast<float>(score_threshold)) - 1.f);

    const auto anchor_matrix_size = _anchor_matrix.size();
    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        const auto   output_scores_id           = _output_scores_ids[i];
        const auto*  output_scores              = output_data[output_scores_id];
        const size_t output_scores_shape_dims_2 = _output_shapes[output_scores_id].dims[2];
        const auto   output_scores_quant_parm   = _output_quant_params[output_scores_id];

        const auto   output_bboxes_id           = _output_bboxes_ids[i];
        const auto*  output_bboxes              = output_data[output_bboxes_id];
        const size_t output_bboxes_shape_dims_2 = _output_shapes[output_bboxes_id].dims[2];
        const auto   output_bboxes_quant_parm   = _output_quant_params[output_bboxes_id];

        const auto  stride  = _scaled_strides[i];
        const float scale_w = stride.first;
        const float scale_h = stride.second;

        const auto& anchor_array      = _anchor_matrix[i];
        const auto  anchor_array_size = anchor_array.size();

        const int32_t score_threshold_quan_non_sigmoid =
          static_cast<int32_t>(std::floor(score_threshold_non_sigmoid / output_scores_quant_parm.scale)) +
          output_scores_quant_parm.zero_point;

        for (size_t j = 0; j < anchor_array_size; ++j) {
            const auto j_mul_output_scores_shape_dims_2 = j * output_scores_shape_dims_2;

            auto    max_score_raw = score_threshold_quan_non_sigmoid;
            int32_t target        = -1;

            for (size_t k = 0; k < output_scores_shape_dims_2; ++k) {
                int8_t score = output_scores[j_mul_output_scores_shape_dims_2 + k];

                if (static_cast<decltype(max_score_raw)>(score) < max_score_raw) [[likely]]
                    continue;

                max_score_raw = score;
                target        = k;
            }

            if (target < 0) continue;

            const float real_score = utils::sigmoid(
              static_cast<float>(max_score_raw - output_scores_quant_parm.zero_point) * output_scores_quant_parm.scale);

            // DFL
            float dist[4];
            float matrix[1];

            const auto pre = j * output_bboxes_shape_dims_2;
            for (size_t m = 0; m < 4; ++m) {
                const size_t offset = pre + m * 1;
                for (size_t n = 0; n < 1; ++n) {
                    matrix[n] = utils::dequant_value_i(
                      offset + n, output_bboxes, output_bboxes_quant_parm.zero_point, output_bboxes_quant_parm.scale);
                }

                // utils::softmax(matrix, 1);

                float res = 0.f;
                for (size_t n = 0; n < 1; ++n) {
                    res += matrix[n] * static_cast<float>(n);
                }
                dist[m] = res;
            }

            const auto anchor = anchor_array[j];

            float cx = anchor.x + ((dist[2] - dist[0]) * 0.5f);
            float cy = anchor.y + ((dist[3] - dist[1]) * 0.5f);
            float w  = dist[0] + dist[2];
            float h  = dist[1] + dist[3];

            _results.emplace_front(el_box_t{
              .x      = static_cast<decltype(BoxType::x)>(cx * scale_w),
              .y      = static_cast<decltype(BoxType::y)>(cy * scale_h),
              .w      = static_cast<decltype(BoxType::w)>(w * scale_w),
              .h      = static_cast<decltype(BoxType::h)>(h * scale_h),
              .score  = static_cast<decltype(BoxType::score)>(real_score * 100.f),
              .target = static_cast<decltype(BoxType::target)>(target),
            });
        }
    }

    el_nms(_results, iou_threshold, score_threshold, false, true);

    return EL_OK;
}

const std::forward_list<AlgorithmRtmdet::BoxType>& AlgorithmRtmdet::get_results() const { return _results; }

void AlgorithmRtmdet::set_score_threshold(ScoreType threshold) { _score_threshold.store(threshold); }

AlgorithmRtmdet::ScoreType AlgorithmRtmdet::get_score_threshold() const { return _score_threshold.load(); }

void AlgorithmRtmdet::set_iou_threshold(IoUType threshold) { _iou_threshold.store(threshold); }

AlgorithmRtmdet::IoUType AlgorithmRtmdet::get_iou_threshold() const { return _iou_threshold.load(); }

void AlgorithmRtmdet::set_algorithm_config(const ConfigType& config) {
    set_score_threshold(config.score_threshold);
    set_iou_threshold(config.iou_threshold);
}

AlgorithmRtmdet::ConfigType AlgorithmRtmdet::get_algorithm_config() const {
    ConfigType config;
    config.score_threshold = get_score_threshold();
    config.iou_threshold   = get_iou_threshold();
    return config;
}

}  // namespace edgelab
