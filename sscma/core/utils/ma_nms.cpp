
#include "ma_nms.h"

#include <algorithm>
#include <cmath>
#include <forward_list>
#include <type_traits>
#include <vector>

namespace ma::utils {

template <typename Container, typename T = typename Container::value_type>
static constexpr void nms_impl(Container& bboxes, float threshold_iou, float threshold_score, bool soft_nms, bool multi_target) {
    if constexpr (std::is_same_v<Container, std::forward_list<T>>) {
        bboxes.sort([](const auto& box1, const auto& box2) { return box1.score > box2.score; });
    } else {
        std::sort(bboxes.begin(), bboxes.end(), [](const auto& box1, const auto& box2) { return box1.score > box2.score; });
    }

    for (auto it = bboxes.begin(); it != bboxes.end(); ++it) {
        if (it->score == 0)
            continue;
        for (auto it2 = std::next(it); it2 != bboxes.end(); ++it2) {
            if (it2->score == 0)
                continue;
            if (multi_target && it->target != it2->target)
                continue;
            const auto iou = compute_iou(*it, *it2);
            if (iou > threshold_iou) {
                if (soft_nms) {
                    it2->score = it2->score * (1 - iou);
                    if (it2->score < threshold_score)
                        it2->score = 0;
                } else {
                    it2->score = 0;
                }
            }
        }
    }

    if constexpr (std::is_same<Container, std::forward_list<T>>::value) {
        bboxes.remove_if([](const auto& box) { return box.score == 0; });
    } else {
        bboxes.erase(std::remove_if(bboxes.begin(), bboxes.end(), [](const auto& box) { return box.score == 0; }), bboxes.end());
    }
}

void nms(std::forward_list<ma_bbox_t>& bboxes, float threshold_iou, float threshold_score, bool soft_nms, bool multi_target) {
    nms_impl(bboxes, threshold_iou, threshold_score, soft_nms, multi_target);
}

void nms(std::forward_list<ma_bbox_ext_t>& bboxes, float threshold_iou, float threshold_score, bool soft_nms, bool multi_target) {
    nms_impl(bboxes, threshold_iou, threshold_score, soft_nms, multi_target);
}

void nms(std::forward_list<ma_keypoint3f_t>& decodings, const float iou_thr, bool should_nms_cross_classes) {
    for (
        auto it = decodings.begin(); it != decodings.end(); ++it) {
        if (it->box.score != 0.0f) {
            for (
                auto it2 = std::next(it); it2 != decodings.end(); ++it2) {
                if ((should_nms_cross_classes || (it->box.target == it2->box.target)) && it2->box.score != 0.0f) {
                    float iou = compute_iou(it->box, it2->box);
                    if (iou >= iou_thr) {
                        it2->box.score = 0.0f;
                    }
                }
            }
        }
    }
    decodings.remove_if([](const auto& box) { return box.box.score == 0.0f; });
}

}  // namespace ma::utils