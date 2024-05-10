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

#ifndef _EL_ALGORITHM_YOLO_POSE_H_
#define _EL_ALGORITHM_YOLO_POSE_H_

#include <atomic>
#include <cstdint>
#include <forward_list>
#include <vector>

#include "core/el_types.h"
#include "el_algorithm_base.h"

namespace edgelab {

using namespace edgelab::base;
using namespace edgelab::types;

namespace types {

// we're not using inheritance since it not standard layout
struct el_algorithm_yolo_pose_config_t {
    static constexpr el_algorithm_info_t info{
      .type = EL_ALGO_TYPE_YOLO_POSE, .categroy = EL_ALGO_CAT_POSE, .input_from = EL_SENSOR_TYPE_CAM};
    uint8_t score_threshold = 70;
    uint8_t iou_threshold   = 45;
};

struct anchor_bbox_t {
    float    x1;
    float    y1;
    float    x2;
    float    y2;
    float    score;
    uint8_t  anchor_class;
    uint16_t anchor_index;
};

template <typename T> struct pt3_t {
    T x;
    T y;
    T z;
};

template <typename T, size_t N> struct pt3_set_t {
    pt3_t<T> data[N];
};


}  // namespace types

class AlgorithmYOLOPOSE final : public Algorithm {
   public:
    using ImageType    = el_img_t;
    using KeyPointType = el_keypoint_t;
    using ConfigType   = el_algorithm_yolo_pose_config_t;
    using ScoreType    = decltype(el_algorithm_yolo_pose_config_t::score_threshold);
    using IoUType      = decltype(el_algorithm_yolo_pose_config_t::iou_threshold);

    static InfoType algorithm_info;

    AlgorithmYOLOPOSE(EngineType* engine, ScoreType score_threshold = 50, IoUType iou_threshold = 45);
    AlgorithmYOLOPOSE(EngineType* engine, const ConfigType& config);
    ~AlgorithmYOLOPOSE();

    static bool is_model_valid(const EngineType* engine);

    el_err_code_t                          run(ImageType* input);
    const std::forward_list<KeyPointType>& get_results() const;

    void      set_score_threshold(ScoreType threshold);
    ScoreType get_score_threshold() const;

    void    set_iou_threshold(IoUType threshold);
    IoUType get_iou_threshold() const;

    void       set_algorithm_config(const ConfigType& config);
    ConfigType get_algorithm_config() const;

   protected:
    inline void init();

    el_err_code_t preprocess() override;
    el_err_code_t postprocess() override;

   private:
    ImageType _input_img;

    decltype(ImageType::width)  _last_input_width;
    decltype(ImageType::height) _last_input_height;

    float _w_scale;
    float _h_scale;

    std::atomic<ScoreType> _score_threshold;
    std::atomic<IoUType>   _iou_threshold;

    std::vector<anchor_stride_t>          _anchor_strides;
    std::vector<std::pair<float, float>>         _scaled_strides;
    std::vector<std::vector<pt_t<float>>> _anchor_matrix;

    static constexpr size_t _outputs         = 7;
    static constexpr size_t _anchor_variants = 3;

    size_t _output_scores_ids[_anchor_variants];
    size_t _output_bboxes_ids[_anchor_variants];
    size_t _output_keypoints_id;

    el_shape_t       _output_shapes[_outputs];
    el_quant_param_t _output_quant_params[_outputs];

    std::forward_list<KeyPointType> _results;
};

}  // namespace edgelab

#endif
