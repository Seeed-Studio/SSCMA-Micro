#ifndef _MA_NMS_H_
#define _MA_NMS_H_

#include <algorithm>
#include <cmath>
#include <forward_list>
#include <iterator>
#include <type_traits>

#include "../ma_types.h"

namespace ma::utils {

// skip use of template since it is not allowed

template <typename T, std::enable_if_t<std::is_base_of<ma_bbox_t, T>::value, bool> = true>
inline float compute_iou(const T& box1, const T& box2) {
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

void nms(std::forward_list<ma_bbox_t>& bboxes, float threshold_iou, float threshold_score, bool soft_nms, bool multi_target);

void nms(std::forward_list<ma_bbox_ext_t>& bboxes, float threshold_iou, float threshold_score, bool soft_nms, bool multi_target);

void nms(std::forward_list<ma_keypoint3f_t>& decodings, const float iou_thr, bool should_nms_cross_classes);

}  // namespace ma::utils

#endif  // _MA_NMS_H_