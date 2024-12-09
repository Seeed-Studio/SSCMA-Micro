#ifndef _MA_MODEL_YOLOV8_SEG_HAILO_H_
#define _MA_MODEL_YOLOV8_SEG_HAILO_H_

#include "ma_model_segmentor.h"

#if MA_USE_ENGINE_HAILO

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <xtensor/xtensor.hpp>
#include <xtensor/xarray.hpp>

namespace ma::model {

namespace _internal {

struct Quadruple {
    std::vector<ma_tensor_t> boxes;
    xt::xarray<float> scores;
    std::vector<ma_tensor_t> masks;
    xt::xarray<float> proto_data;
};

}  // namespace _internal

class YoloV8SegHailo : public Segmentor {
private:
    std::vector<xt::xarray<double>> centers_;
    static std::vector<int> strides_;
    std::vector<int> network_dims_;
    std::vector<ma_tensor_t> outputs_;
    _internal::Quadruple boxes_scores_masks_mask_matrix_;
    int classes_ = 0;
    int32_t route_ = 0;

protected:
    ma_err_t postprocess();

public:
    YoloV8SegHailo(Engine* engine);
    ~YoloV8SegHailo();

    static bool isValid(Engine* engine);

    static const char* getTag();
};

}  // namespace ma::model

#endif

#endif  // _MA_MODEL_YOLO_H
