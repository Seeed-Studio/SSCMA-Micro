#include "ma_model_yolov8_seg_hailo.h"

#if MA_USE_ENGINE_HAILO

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <forward_list>
#include <numeric>
#include <utility>
#include <vector>

#include <xtensor/xadapt.hpp>
#include <xtensor/xarray.hpp>
#include <xtensor/xmath.hpp>
#include <xtensor/xsort.hpp>
#include <xtensor/xtensor.hpp>
#include <xtensor/xview.hpp>

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

static void nms(std::forward_list<std::pair<ma_bbox_t, xt::xarray<float>>>& decodings, const float iou_thr, bool should_nms_cross_classes) {
    for (auto it = decodings.begin(); it != decodings.end(); ++it) {
        if (it->first.score != 0.0f) {
            for (auto it2 = std::next(it); it2 != decodings.end(); ++it2) {
                if ((should_nms_cross_classes || (it->first.target == it2->first.target)) && it2->first.score != 0.0f) {
                    float iou = ma::utils::compute_iou(it->first, it2->first);
                    if (iou >= iou_thr) {
                        it2->first.score = 0.0f;
                    }
                }
            }
        }
    }
    decodings.remove_if([](const auto& p) { return p.first.score == 0.0f; });
}

std::vector<int> YoloV8SegHailo::strides_ = {8, 16, 32};

/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
static decltype(auto) getBoxesScoresMasks(std::vector<ma_tensor_t> tensors, int num_classes) {
    auto raw_proto = [&tensors]() {
        auto it = std::find_if(
            tensors.begin(), tensors.end(), [](const ma_tensor_t& t) { return t.size == 4 && t.shape.dims[0] == 1 && t.shape.dims[1] == 160 && t.shape.dims[2] == 160 && t.shape.dims[3] == 32; });
        if (it == tensors.end()) {
            return ma_tensor_t{0};
        }
        tensors.erase(it);
        return *it;
    }();

    std::vector<ma_tensor_t> outputs_boxes(tensors.size() / 3);
    std::vector<ma_tensor_t> outputs_masks(tensors.size() / 3);

    int total_scores = 0;
    for (uint i = 0; i < tensors.size(); i = i + 3) {
        auto w = tensors[i + 1].shape.dims[1];  // w
        auto h = tensors[i + 1].shape.dims[2];  // h
        total_scores += w * h;
    }

    std::vector<size_t> scores_shape = {(long unsigned int)total_scores, (long unsigned int)num_classes};
    xt::xarray<float> scores(scores_shape);

    std::vector<size_t> proto_shape = {(long unsigned int)raw_proto.shape.dims[1], (long unsigned int)raw_proto.shape.dims[2], (long unsigned int)raw_proto.shape.dims[3]};
    xt::xarray<float> proto(proto_shape);

    int view_index_scores = 0;

    for (uint i = 0; i < tensors.size(); i = i + 3) {
        outputs_boxes[i / 3] = tensors[i];

        auto& tensor                = tensors[i + 1];
        std::vector<size_t> shape   = {(size_t)tensor.shape.dims[1], (size_t)tensor.shape.dims[2], (size_t)tensor.shape.dims[3]};
        xt::xarray<uint8_t> xtensor = xt::adapt(tensor.data.u8, tensor.size, xt::no_ownership(), shape);
        auto dequantized_output_s   = (xtensor - tensor.quant_param.zero_point) * tensor.quant_param.scale;
        int num_proposals_scores    = dequantized_output_s.shape(0) * dequantized_output_s.shape(1);

        auto output_scores                                                                                  = xt::view(dequantized_output_s, xt::all(), xt::all(), xt::all());
        xt::view(scores, xt::range(view_index_scores, view_index_scores + num_proposals_scores), xt::all()) = xt::reshape_view(output_scores, {num_proposals_scores, num_classes});
        view_index_scores += num_proposals_scores;

        outputs_masks[i / 3] = tensors[i + 2];
    }

    auto proto_tensor = xt::adapt(raw_proto.data.u8, raw_proto.size, xt::no_ownership(), proto_shape);
    proto             = (proto_tensor - raw_proto.quant_param.zero_point) * raw_proto.quant_param.scale;


    return _internal::Quadruple{outputs_boxes, scores, outputs_masks, proto};
}


YoloV8SegHailo::YoloV8SegHailo(Engine* p_engine_) : Segmentor(p_engine_, "yolov8_seg", MA_MODEL_TYPE_YOLOV8_SGE) {
    MA_ASSERT(p_engine_ != nullptr);

    threshold_score_ = 0.6;
    threshold_nms_   = 0.7;

    outputs_.resize(10);
    for (size_t i = 0; i < outputs_.size(); ++i) {
        outputs_[i] = p_engine_->getOutput(i);
    }

    std::sort(outputs_.begin(), outputs_.end(), [](const ma_tensor_t& a, const ma_tensor_t& b) { return a.shape.dims[1] > b.shape.dims[1]; });

    auto update_route_f = [&route = route_, this](ma_tensor_type_t t, int i) {
        switch (t) {
            case MA_TENSOR_TYPE_U8:
                route |= 1 << i;
                break;
            case MA_TENSOR_TYPE_U16:
                route |= 1 << (i + this->outputs_.size());
                break;
            default:
                break;
        }
    };

    std::vector<size_t> idx(outputs_.size());
    for (size_t i = 1; i < outputs_.size(); i += 3) {
        for (size_t j = 0; j < 3; ++j) {
            auto at = i + j;
            switch (outputs_[at].shape.dims[3]) {
                case 32:
                    idx[i + 2] = at;
                    break;
                case 64:
                    idx[i] = at;
                    break;
                default:
                    idx[i + 1] = at;
            }
        }
    }
    std::vector<ma_tensor_t> reordered_outputs(outputs_.size());
    reordered_outputs[0] = outputs_[0];
    update_route_f(reordered_outputs[0].type, 0);
    for (size_t i = 1; i < outputs_.size(); ++i) {
        reordered_outputs[i] = outputs_[idx[i]];
        update_route_f(reordered_outputs[i].type, i);
    }
    outputs_ = std::move(reordered_outputs);
    classes_ = outputs_[2].shape.dims[3];

    const auto [h, w] = estimateTensorHW(p_engine_->getInputShape(0));

    centers_      = ma::utils::generateAnchorMatrix(strides_, {static_cast<int>(w), static_cast<int>(h)}, 3, 0, 0);
    network_dims_ = {w, h};
}

YoloV8SegHailo::~YoloV8SegHailo() {}

bool YoloV8SegHailo::isValid(Engine* engine) {
    const auto inputs_count  = engine->getInputSize();
    const auto outputs_count = engine->getOutputSize();

    if (inputs_count != 1 || outputs_count != 10) {
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

    auto it = std::find_if(
        outputs.begin(), outputs.end(), [](const ma_tensor_t& t) { return t.size == 4 && t.shape.dims[0] == 1 && t.shape.dims[1] == 160 && t.shape.dims[2] == 160 && t.shape.dims[3] == 32; });
    if (it == outputs.end()) {
        return false;
    }
    outputs.erase(it);

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
            case 32:
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
                if (out.type != MA_TENSOR_TYPE_U8) {
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

const char* YoloV8SegHailo::getTag() {
    return "ma::model::yolov8_pose";
}

template <typename T>
static decltype(auto) decodeBoxesAndExtractMasks(const std::vector<ma_tensor_t>& raw_boxes_outputs,
                                                 const std::vector<ma_tensor_t>& raw_masks_outputs,
                                                 xt::xarray<float>& scores,
                                                 const std::vector<int>& network_dims,
                                                 const std::vector<int>& strides,
                                                 const std::vector<xt::xarray<double>>& centers,
                                                 int regression_length,
                                                 float score_threshold) {

    int class_index = 0;
    std::forward_list<std::pair<ma_bbox_t, xt::xarray<float>>> decodings;

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
        auto output_b                      = xt::adapt(raw_boxes_outputs[i].data.u8, raw_boxes_outputs[i].size, xt::no_ownership(), output_b_shape);

        int num_proposals    = output_b.shape(0) * output_b.shape(1);
        auto output_boxes    = xt::view(output_b, xt::all(), xt::all(), xt::all());
        auto quantized_boxes = xt::reshape_view(output_boxes, {num_proposals, 4, regression_length + 1});

        auto shape = {quantized_boxes.shape(1), quantized_boxes.shape(2)};

        // Masks setup
        float32_t qp_scale_masks = raw_masks_outputs[i].quant_param.scale;
        float32_t qp_zp_masks    = raw_masks_outputs[i].quant_param.zero_point;

        std::vector<size_t> output_m_shape = {(size_t)raw_masks_outputs[i].shape.dims[1], (size_t)raw_masks_outputs[i].shape.dims[2], (size_t)raw_masks_outputs[i].shape.dims[3]};
        auto output_m                      = xt::adapt(raw_masks_outputs[i].data.u8, raw_masks_outputs[i].size, xt::no_ownership(), output_m_shape);

        int num_proposals_masks = output_m.shape(0) * output_m.shape(1);
        auto output_masks       = xt::view(output_m, xt::all(), xt::all(), xt::all());
        auto quantized_masks    = xt::reshape_view(output_masks, {num_proposals_masks, 1, regression_length + 1});

        auto mask_shape = {quantized_masks.shape(1)};

        // Bbox decoding
        for (uint j = 0; j < (uint)num_proposals; ++j) {
            class_index = xt::argmax(xt::row(scores, instance_index))(0);
            confidence  = scores(instance_index, class_index);
            instance_index++;
            if (confidence < score_threshold)
                continue;

            xt::xarray<float> box(shape);
            xt::xarray<float> mask(mask_shape);

            ma::math::dequantizeValues2D<uint8_t>(box, j, quantized_boxes, box.shape(0), box.shape(1), qp_scale, qp_zp);
            ma::math::softmax2D(box.data(), box.shape(0), box.shape(1));

            ma::math::dequantizeValues1D<uint8_t>(mask, j, quantized_masks, mask.shape(0), qp_scale_masks, qp_zp_masks);

            auto box_distance                   = box * regression_distance;
            xt::xarray<float> reduced_distances = xt::sum(box_distance, {2});
            auto strided_distances              = reduced_distances * strides[i];

            using namespace xt::placeholders;
            auto distance_view1 = xt::view(strided_distances, xt::all(), xt::range(_, 2)) * -1;
            auto distance_view2 = xt::view(strided_distances, xt::all(), xt::range(2, _));
            auto distance_view  = xt::concatenate(xt::xtuple(distance_view1, distance_view2), 1);
            auto decoded_box    = centers[i] + distance_view;

            ma_bbox_t bbox;
            auto x_min = decoded_box(j, 0) / network_dims[0];
            auto y_min = decoded_box(j, 1) / network_dims[1];
            auto w     = (decoded_box(j, 2) - decoded_box(j, 0)) / network_dims[0];
            auto h     = (decoded_box(j, 3) - decoded_box(j, 1)) / network_dims[1];

            bbox.x      = x_min + (w / 2);
            bbox.y      = y_min + (h / 2);
            bbox.w      = w;
            bbox.h      = h;
            bbox.score  = confidence;
            bbox.target = class_index;

            decodings.emplace_front(std::make_pair(bbox, mask));
        }
    }

    return decodings;
}


static xt::xarray<float> dot(xt::xarray<float> mask, xt::xarray<float> reshaped_proto, size_t proto_height, size_t proto_width, size_t mask_num = 32) {

    auto shape = {proto_height, proto_width};
    xt::xarray<float> mask_product(shape);

    for (size_t i = 0; i < mask_product.shape(0); i++) {
        for (size_t j = 0; j < mask_product.shape(1); j++) {
            for (size_t k = 0; k < mask_num; k++) {
                mask_product(i, j) += mask(k) * reshaped_proto(k, i, j);
            }
        }
    }
    return mask_product;
}

ma_err_t YoloV8SegHailo::postprocess() {
    // TODO: could be optimized

    boxes_scores_masks_mask_matrix_ = getBoxesScoresMasks(outputs_, classes_);
    std::forward_list<std::pair<ma_bbox_t, xt::xarray<float>>> decodings;

    switch (route_) {
        case 1023:
            decodings = decodeBoxesAndExtractMasks<uint8_t>(
                boxes_scores_masks_mask_matrix_.boxes, boxes_scores_masks_mask_matrix_.masks, boxes_scores_masks_mask_matrix_.scores, network_dims_, strides_, centers_, 15, threshold_score_);
            break;
        default:
            return MA_ENOTSUP;
    }

    nms(decodings, threshold_nms_, true);

    xt::xarray<float> proto = boxes_scores_masks_mask_matrix_.proto_data;
    int mask_height         = static_cast<int>(proto.shape(0));
    int mask_width          = static_cast<int>(proto.shape(1));
    int mask_features       = static_cast<int>(proto.shape(2));
    auto reshaped_proto     = xt::reshape_view(xt::transpose(xt::reshape_view(proto, {-1, mask_features}), {1, 0}), {-1, mask_height, mask_width});

    for (const auto& [bbox, curr_mask] : decodings) {
        ma_segm2f_t segm;
        segm.box = bbox;

        auto mask_product = dot(curr_mask, reshaped_proto, reshaped_proto.shape(1), reshaped_proto.shape(2), curr_mask.shape(0));
        for (auto& v : mask_product) {
            v = ma::math::sigmoid(v);
        }

        int x1 = (bbox.x - bbox.w / 2) * mask_width;
        int y1 = (bbox.y - bbox.h / 2) * mask_height;
        int x2 = (bbox.x + bbox.w / 2) * mask_width;
        int y2 = (bbox.y + bbox.h / 2) * mask_height;

        segm.mask.width  = mask_width;
        segm.mask.height = mask_height;
        auto sz = mask_width * mask_height;
        segm.mask.data.resize(static_cast<size_t>(std::ceil(static_cast<float>(sz) / 8.f)), 0);  // bitwise

        for (int i = y1; i < y2; ++i) {
            for (int j = x1; j < x2; ++j) {
                if (mask_product(i, j) > 0.5) {
                    segm.mask.data[i / 8] |= 1 << (i % 8);
                }
            }
        } 

        results_.emplace_front(std::move(segm));
    }

    return MA_OK;
}

}  // namespace ma::model

#endif