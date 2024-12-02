#include "ma_model_yolov8_pose_hailo.h"

#if MA_USE_ENGINE_HAILO

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <numeric>
#include <utility>
#include <vector>

#include <xtensor/xarray.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xmath.hpp>
#include <xtensor/xsort.hpp>
#include <xtensor/xadapt.hpp>
#include <xtensor/xtensor.hpp>

#include "../math/ma_math.h"
#include "../utils/ma_anchors.h"
#include "../utils/ma_nms.h"

namespace ma::model {

static inline decltype(auto) estimateTensorHW(const ma_shape_t& shape) {
    if (shape.size != 4) {
        int32_t ph = 0;
        return std::make_pair(ph, ph);
    }
    const auto is_nhwc{shape.dims[3] == 3 || shape.dims[3] == 1};

    return is_nhwc ? std::make_pair(shape.dims[1], shape.dims[2]) : std::make_pair(shape.dims[2], shape.dims[3]);
}

std::vector<int> YoloV8PoseHailo::strides_ = {8, 16, 32};

/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
static decltype(auto) getBoxesScoresKeypoints(std::vector<ma_tensor_t>& tensors, int num_classes) {
    std::vector<ma_tensor_t> outputs_boxes(tensors.size() / 3);
    std::vector<ma_tensor_t> outputs_keypoints(tensors.size() / 3);

    int total_scores = 0;
    for (uint i = 0; i < tensors.size(); i = i + 3) {
        auto w = tensors[i + 1].shape.dims[1];  // w
        auto h = tensors[i + 1].shape.dims[2];  // h
        total_scores += w * h;
    }

    std::vector<size_t> scores_shape = {(long unsigned int)total_scores, (long unsigned int)num_classes};

    xt::xarray<float> scores(scores_shape);

    int view_index_scores = 0;

    for (uint i = 0; i < tensors.size(); i = i + 3) {
        outputs_boxes[i / 3] = tensors[i];

        auto& tensor                = tensors[i + 1];
        std::vector<size_t> shape   = {(size_t)tensor.shape.dims[1], (size_t)tensor.shape.dims[2], (size_t)tensor.shape.dims[3]};
        xt::xarray<uint8_t> xtensor = xt::adapt(tensor.data.u8, tensor.size, xt::no_ownership(), shape);
        auto dequantized_output_s   = (xtensor - tensor.quant_param.zero_point) * tensor.quant_param.scale;

        int num_proposals_scores = dequantized_output_s.shape(0) * dequantized_output_s.shape(1);

        auto output_scores                                                                                  = xt::view(dequantized_output_s, xt::all(), xt::all(), xt::all());
        xt::view(scores, xt::range(view_index_scores, view_index_scores + num_proposals_scores), xt::all()) = xt::reshape_view(output_scores, {num_proposals_scores, num_classes});
        view_index_scores += num_proposals_scores;

        outputs_keypoints[i / 3] = tensors[i + 2];
    }

    return _internal::Triple{outputs_boxes, scores, outputs_keypoints};
}


YoloV8PoseHailo::YoloV8PoseHailo(Engine* p_engine_) : PoseDetector(p_engine_, "yolov8_pose", MA_MODEL_TYPE_YOLOV8_POSE) {
    MA_ASSERT(p_engine_ != nullptr);

    threshold_score_ = 0.6;
    threshold_nms_   = 0.7;

    outputs_.resize(9);
    for (size_t i = 0; i < outputs_.size(); ++i) {
        outputs_[i] = p_engine_->getOutput(i);
    }

    std::sort(outputs_.begin(), outputs_.end(), [](const ma_tensor_t& a, const ma_tensor_t& b) { return a.shape.dims[1] > b.shape.dims[1]; });
   
    auto update_route_f = [&route = route_](ma_tensor_type_t t, int i) {
        switch (t) {
            case MA_TENSOR_TYPE_U8:
                route |= 1 << i;
                break;
            case MA_TENSOR_TYPE_U16:
                route |= 1 << (i + 9);
                break;
            default:
                break;
        }
    };

    std::vector<size_t> idx(outputs_.size());
    for (size_t i = 0; i < outputs_.size(); i += 3) {
        for (size_t j = 0; j < 3; ++j) {
            auto at = i + j;
            switch (outputs_[at].shape.dims[3]) {
                case 1:
                    idx[i + 1] = at;
                    break;
                case 64:
                    idx[i] = at;
                    break;
                default:
                    idx[i + 2] = at;
            }
        }
    }
    std::vector<ma_tensor_t> reordered_outputs(outputs_.size()); 
    for (size_t i = 0; i < outputs_.size(); ++i) {
        reordered_outputs[i] = outputs_[idx[i]];
        update_route_f(reordered_outputs[i].type, i);
    }
    outputs_ = std::move(reordered_outputs);

    const auto [h, w] = estimateTensorHW(p_engine_->getInputShape(0));

    centers_      = ma::utils::generateAnchorMatrix(strides_, {static_cast<int>(w), static_cast<int>(h)}, 3, 0, 0);
    network_dims_ = {w, h};
}

YoloV8PoseHailo::~YoloV8PoseHailo() {}

bool YoloV8PoseHailo::isValid(Engine* engine) {
    const auto inputs_count  = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();

    if (inputs_count != 1 || outputs_count != 9) {
        return false;
    }

    const auto input_shape{engine->getInputShape(0)};

    if (input_shape.size != 4) {
        return false;
    }

    const auto is_nhwc{input_shape.dims[3] == 3 || input_shape.dims[3] == 1};

    size_t n = 0, h = 0, w = 0, c = 0;

    if (is_nhwc) {
        n = input_shape.dims[0];
        h = input_shape.dims[1];
        w = input_shape.dims[2];
        c = input_shape.dims[3];
    } else {
        n = input_shape.dims[0];
        c = input_shape.dims[1];
        h = input_shape.dims[2];
        w = input_shape.dims[3];
    }

    if (n != 1 || h ^ w || h < 32 || h % 32 || (c != 3 && c != 1)) {
        return false;
    }

    const auto output_nums = engine->getOutputSize();
    if (output_nums != 9) {
        return false;
    }

    std::vector<ma_tensor_t> outputs(output_nums);
    for (size_t i = 0; i < output_nums; ++i) {
        outputs[i] = engine->getOutput(i);
    }

    std::vector<std::vector<int>> dims{std::vector<int>{int(w / strides_[0]), int(h / strides_[0]), 0},
                                       std::vector<int>{int(w / strides_[1]), int(h / strides_[1]), 0},
                                       std::vector<int>{int(w / strides_[2]), int(h / strides_[2]), 0}};

    for (auto& out : outputs) {
        if (out.shape.size != 4 || out.shape.dims[0] != 1) {
            return false;
        }
        auto it = std::find_if(dims.begin(), dims.end(), [&out](const std::vector<int>& dim) { return dim[0] == out.shape.dims[1] && dim[1] == out.shape.dims[2]; });
        if (it == dims.end()) {
            return false;
        }
        switch (out.shape.dims[3]) {
            case 1:
                if (out.type != MA_TENSOR_TYPE_U8) {
                    return false;
                }
                (*it)[2] += 1;
                break;
            case 64:
                if (out.type != MA_TENSOR_TYPE_U8) {
                    return false;
                }
                (*it)[2] += 1;
                break;
            default:
                if (out.shape.dims[3] % 3 != 0) {
                    return false;
                }
                if (out.type != MA_TENSOR_TYPE_U8 && out.type != MA_TENSOR_TYPE_U16) {
                    return false;
                }
                (*it)[2] += 1;
        }
    }

    for (const auto& dim : dims) {
        if (dim[2] != 3) {
            return false;
        }
    }

    return true;
}

const char* YoloV8PoseHailo::getTag() {
    return "ma::model::yolov8_pose";
}

template <typename KptsType>
static decltype(auto) decodeBoxesAndKeypoints(const std::vector<ma_tensor_t>& raw_boxes_outputs,
                                              xt::xarray<float>& scores,
                                              const std::vector<ma_tensor_t>& raw_keypoints,
                                              const std::vector<int>& network_dims,
                                              const std::vector<int>& strides,
                                              const std::vector<xt::xarray<double>>& centers,
                                              int regression_length,
                                              float score_threshold) {

    int class_index = 0;
    std::forward_list<ma_keypoint3f_t> decodings;

    int instance_index = 0;
    float confidence   = 0.0;
    std::string label;

    // Box distribution to distance
    auto regression_distance = xt::reshape_view(xt::arange(0, regression_length + 1), {1, 1, regression_length + 1});

    for (uint i = 0; i < raw_boxes_outputs.size(); ++i) {
        // Boxes setup
        float32_t qp_scale = raw_boxes_outputs[i].quant_param.scale;
        float32_t qp_zp    = raw_boxes_outputs[i].quant_param.zero_point;

        std::vector<size_t> output_b_shape = {(size_t)raw_boxes_outputs[i].shape.dims[1], (size_t)raw_boxes_outputs[i].shape.dims[2], (size_t)raw_boxes_outputs[i].shape.dims[3]};
        auto output_b = xt::adapt(raw_boxes_outputs[i].data.u8, raw_boxes_outputs[i].size, xt::no_ownership(), output_b_shape);

        int num_proposals    = output_b.shape(0) * output_b.shape(1);
        auto output_boxes    = xt::view(output_b, xt::all(), xt::all(), xt::all());
        auto quantized_boxes = xt::reshape_view(output_boxes, {num_proposals, 4, regression_length + 1});

        auto shape = {quantized_boxes.shape(1), quantized_boxes.shape(2)};

        // Keypoints setup
        float32_t qp_scale_kpts = raw_keypoints[i].quant_param.scale;
        float32_t qp_zp_kpts    = raw_keypoints[i].quant_param.zero_point;

        std::vector<size_t> output_keypoints_shape = {(size_t)raw_keypoints[i].shape.dims[1], (size_t)raw_keypoints[i].shape.dims[2], (size_t)raw_keypoints[i].shape.dims[3]};
       
        size_t output_keypoints_size = output_keypoints_shape[0] * output_keypoints_shape[1] * output_keypoints_shape[2];
        auto output_keypoints        = xt::adapt(static_cast<KptsType*>(raw_keypoints[i].data.data), output_keypoints_size, xt::no_ownership(), output_keypoints_shape);

        int num_proposals_keypoints     = output_keypoints.shape(0) * output_keypoints.shape(1);
        auto output_keypoints_quantized = xt::view(output_keypoints, xt::all(), xt::all(), xt::all());
        auto quantized_keypoints        = xt::reshape_view(output_keypoints_quantized, {num_proposals_keypoints, int(output_keypoints_shape[2] / 3), 3});

        auto keypoints_shape = {quantized_keypoints.shape(1), quantized_keypoints.shape(2)};

        // Bbox decoding
        for (uint j = 0; j < (uint)num_proposals; ++j) {
            confidence = xt::row(scores, instance_index)(0);
            instance_index++;
            if (confidence < score_threshold)
                continue;

            xt::xarray<float> box(shape);
            xt::xarray<float> kpts_corrdinates_and_scores(keypoints_shape);

            ma::math::dequantizeValues2D<uint8_t>(box, j, quantized_boxes, box.shape(0), box.shape(1), qp_scale, qp_zp);
            ma::math::softmax2D(box.data(), box.shape(0), box.shape(1));

            auto box_distance                   = box * regression_distance;
            xt::xarray<float> reduced_distances = xt::sum(box_distance, {2});
            auto strided_distances              = reduced_distances * strides[i];

            using namespace xt::placeholders;
            auto distance_view1 = xt::view(strided_distances, xt::all(), xt::range(_, 2)) * -1;
            auto distance_view2 = xt::view(strided_distances, xt::all(), xt::range(2, _));
            auto distance_view  = xt::concatenate(xt::xtuple(distance_view1, distance_view2), 1);
            auto decoded_box    = centers[i] + distance_view;

            ma_keypoint3f_t kp;
            auto x_min = decoded_box(j, 0) / network_dims[0];
            auto y_min = decoded_box(j, 1) / network_dims[1];
            auto w     = (decoded_box(j, 2) - decoded_box(j, 0)) / network_dims[0];
            auto h     = (decoded_box(j, 3) - decoded_box(j, 1)) / network_dims[1];

            kp.box.x      = x_min + (w / 2);
            kp.box.y      = y_min + (h / 2);
            kp.box.w      = w;
            kp.box.h      = h;
            kp.box.score  = confidence;
            kp.box.target = class_index;

            ma::math::dequantizeValues2D<KptsType>(
                kpts_corrdinates_and_scores, j, quantized_keypoints, kpts_corrdinates_and_scores.shape(0), kpts_corrdinates_and_scores.shape(1), qp_scale_kpts, qp_zp_kpts);

            auto kpts_corrdinates = xt::view(kpts_corrdinates_and_scores, xt::all(), xt::range(0, 2));
            auto keypoints_scores = xt::view(kpts_corrdinates_and_scores, xt::all(), xt::range(2, _));

            kpts_corrdinates *= 2;

            auto center        = xt::view(centers[i], xt::all(), xt::range(0, 2));
            auto center_values = xt::xarray<float>{(float)center(j, 0), (float)center(j, 1)};

            kpts_corrdinates = strides[i] * (kpts_corrdinates - 0.5) + center_values;

            auto sigmoided_scores = 1 / (1 + xt::exp(-keypoints_scores));

            auto keypoint = std::make_pair(kpts_corrdinates, sigmoided_scores);

            int pt_size = kpts_corrdinates.shape(0);
            for (int i = 0; i < pt_size; ++i) {
                ma_pt3f_t pt;
                pt.x = kpts_corrdinates(i, 0) / network_dims[0];
                pt.y = kpts_corrdinates(i, 1) / network_dims[1];
                pt.z = sigmoided_scores(i, 0);
                kp.pts.push_back(pt);
            }

            decodings.push_front(std::move(kp));
        }
    }

    return decodings;
}


ma_err_t YoloV8PoseHailo::postprocess() {
    // TODO: could be optimized
    boxes_scores_keypoints_ = getBoxesScoresKeypoints(outputs_, 1);

    switch (route_) {
        case 511:
            results_ = decodeBoxesAndKeypoints<uint8_t>(
                boxes_scores_keypoints_.boxes, boxes_scores_keypoints_.scores, boxes_scores_keypoints_.keypoints, network_dims_, strides_, centers_, 15, threshold_score_);
            break;
        case 149723:
            results_ = decodeBoxesAndKeypoints<uint16_t>(
                boxes_scores_keypoints_.boxes, boxes_scores_keypoints_.scores, boxes_scores_keypoints_.keypoints, network_dims_, strides_, centers_, 15, threshold_score_);
            break;
        default:
            return MA_ENOTSUP;
    }

    ma::utils::nms(results_, threshold_nms_, true);

    return MA_OK;
}

}  // namespace ma::model

#endif