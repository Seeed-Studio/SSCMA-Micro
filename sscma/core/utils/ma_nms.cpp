
#include "ma_nms.h"

#include <algorithm>
#include <cmath>
#include <forward_list>
#include <type_traits>
#include <vector>

namespace ma::utils {

template <typename T, std::enable_if_t<std::is_base_of_v<ma_bbox_t, T>, bool> = true>
static inline float compute_iou(const T& box1, const T& box2) {
    const float x1    = std::max(box1.x, box2.x);
    const float y1    = std::max(box1.y, box2.y);
    const float x2    = std::min(box1.x + box1.w, box2.x + box2.w);
    const float y2    = std::min(box1.y + box1.h, box2.y + box2.h);
    const float w     = std::max(0.0f, x2 - x1);
    const float h     = std::max(0.0f, y2 - y1);
    const float inter = w * h;
    const float d     = box1.w * box1.h + box2.w * box2.h - inter;
    if (std::abs(d) < std::numeric_limits<float>::epsilon()) [[unlikely]] {
        return 0;
    }
    return inter / d;
}

template <typename Container, typename T = typename Container::value_type>
static constexpr void nms_impl(Container& bboxes, float threshold_iou, float threshold_score, bool soft_nms, bool multi_target) {
    if constexpr (std::is_same_v<Container, std::forward_list<T>>) {
        bboxes.sort([](const auto& box1, const auto& box2) { return box1.score > box2.score; });
    } else {
        std::sort(
          bboxes.begin(), bboxes.end(), [](const auto& box1, const auto& box2) { return box1.score > box2.score; });
    }

    for (auto it = bboxes.begin(); it != bboxes.end(); ++it) {
        if (it->score == 0) continue;
        for (auto it2 = std::next(it); it2 != bboxes.end(); ++it2) {
            if (it2->score == 0) continue;
            if (multi_target && it->target != it2->target) continue;
            const auto iou = compute_iou(*it, *it2);
            if (iou > threshold_iou) {
                if (soft_nms) {
                    it2->score = it2->score * (1 - iou);
                    if (it2->score < threshold_score) it2->score = 0;
                } else {
                    it2->score = 0;
                }
            }
        }
    }

    if constexpr (std::is_same_v<Container, std::forward_list<T>>) {
        bboxes.remove_if([](const auto& box) { return box.score == 0; });
    } else {
        bboxes.erase(std::remove_if(bboxes.begin(), bboxes.end(), [](const auto& box) { return box.score == 0; }),
                     bboxes.end());
    }
}

void nms(
  std::forward_list<ma_bbox_t>& bboxes, float threshold_iou, float threshold_score, bool soft_nms, bool multi_target) {
    nms_impl(bboxes, threshold_iou, threshold_score, soft_nms, multi_target);
}

void nms(std::forward_list<ma_bbox_ext_t>& bboxes,
         float                             threshold_iou,
         float                             threshold_score,
         bool                              soft_nms,
         bool                              multi_target) {
    nms_impl(bboxes, threshold_iou, threshold_score, soft_nms, multi_target);
}

}  // namespace ma::utils