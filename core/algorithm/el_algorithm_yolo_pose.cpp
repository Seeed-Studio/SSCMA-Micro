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

    auto loc_len{[&]() {
        auto r{static_cast<uint16_t>(input_shape.dims[1])};
        auto s{r >> 5};  // r / 32
        auto m{r >> 4};  // r / 16
        auto l{r >> 3};  // r / 8
        return s * s + m * m + l * l;
    }()};

    const auto& output_shape{engine->get_output_shape(0)};
    if (output_shape.size != 3 ||               // B, IB, BC...
        output_shape.dims[0] != 1 ||            // B = 1
        (output_shape.dims[1] - 5) % 3 != 0 ||  // x, y, w, h, s, (x, y, v)...
        output_shape.dims[2] != loc_len)
        return false;

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

el_err_code_t AlgorithmYOLOPOSE::postprocess() {
    _results.clear();

    // get output
    auto* data{static_cast<int8_t*>(this->__p_engine->get_output(0))};

    const auto width{this->__input_shape.dims[1]};
    const auto height{this->__input_shape.dims[2]};

    const float scale{this->__output_quant.scale};
    const bool  rescale{scale < 0.1f ? true : false};

    const int32_t zero_point{this->__output_quant.zero_point};

    // shape: 1, 56, 756
    // idx  : 0,  1,   2
    const std::size_t num_types{this->__output_shape.dims[1]};  // 56
    const std::size_t num_pts{this->__output_shape.dims[2]};    // 756
    const std::size_t box_strides[]{
      INDEX_X,            // cx, 0
      INDEX_Y * num_pts,  // cy, 1
      INDEX_W * num_pts,  // w , 2
      INDEX_H * num_pts,  // h , 3
      INDEX_S * num_pts,  // s , 4
    };

    ScoreType score_threshold{get_score_threshold()};
    IoUType   iou_threshold{get_iou_threshold()};

    using BoxType = el_box_t;
    std::forward_list<BoxType> objects;
    for (std::size_t i = 0; i < num_pts; ++i) {
        auto score{static_cast<decltype(scale)>(data[i + box_strides[INDEX_S]] - zero_point) * scale};

        if (score < score_threshold) continue;

        BoxType box{
          .x      = 0,
          .y      = 0,
          .w      = 0,
          .h      = 0,
          .score  = static_cast<decltype(BoxType::score)>(score),
          .target = static_cast<decltype(BoxType::target)>(i),
        };

        auto x{static_cast<decltype(BoxType::x)>((data[i + box_strides[INDEX_X]] - zero_point) * scale)};
        auto y{static_cast<decltype(BoxType::y)>((data[i + box_strides[INDEX_Y]] - zero_point) * scale)};
        auto w{static_cast<decltype(BoxType::w)>((data[i + box_strides[INDEX_W]] - zero_point) * scale)};
        auto h{static_cast<decltype(BoxType::h)>((data[i + box_strides[INDEX_H]] - zero_point) * scale)};

        box.x = EL_CLIP(x, 0, width) * _w_scale;
        box.y = EL_CLIP(y, 0, height) * _h_scale;
        box.w = EL_CLIP(w, 0, width) * _w_scale;
        box.h = EL_CLIP(h, 0, height) * _h_scale;

        objects.emplace_front(std::move(box));
    }

    el_nms(objects, iou_threshold, score_threshold, false, false);

    if (!std::distance(objects.begin(), objects.end())) return EL_OK;

    std::size_t              pts_views{num_types - 5};
    std::vector<std::size_t> pts_strides(pts_views);
    for (std::size_t i = 0; i < pts_views; ++i) {
        pts_strides[i] = (i + 5) * num_pts;
    }

    pts_views /= 3;
    decltype(KeyPointType::target) target = 0;
    for (const auto& obj : objects) {
        KeyPointType keypoint_tl{
          .x      = static_cast<decltype(KeyPointType::x)>(obj.x - (obj.w >> 1)),
          .y      = static_cast<decltype(KeyPointType::y)>(obj.y - (obj.h >> 1)),
          .score  = obj.score,
          .target = target,
        };
        KeyPointType keypoint_br{
          .x      = static_cast<decltype(KeyPointType::x)>(keypoint_tl.x + obj.w),
          .y      = static_cast<decltype(KeyPointType::y)>(keypoint_tl.y + obj.h),
          .score  = obj.score,
          .target = target,
        };
        _results.emplace_front(std::move(keypoint_tl));
        _results.emplace_front(std::move(keypoint_br));

        auto idx = obj.target;
        for (std::size_t i = 0; i < pts_views; ++i) {
            auto         offset{i * 3};
            KeyPointType keypoint{
              .x      = 0,
              .y      = 0,
              .score  = 0,
              .target = target,
            };

            auto x{static_cast<decltype(KeyPointType::x)>((data[idx + pts_strides[offset]] - zero_point) * scale)};
            auto y{static_cast<decltype(KeyPointType::y)>((data[idx + pts_strides[offset + 1]] - zero_point) * scale)};
            auto v{
              static_cast<decltype(KeyPointType::score)>((data[idx + pts_strides[offset + 2]] - zero_point) * scale)};
            x = EL_CLIP(x, 0, width) * _w_scale;
            y = EL_CLIP(y, 0, height) * _h_scale;

            keypoint.x     = x;
            keypoint.y     = y;
            keypoint.score = v;

            _results.emplace_front(std::move(keypoint));
        }

        ++idx;
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
