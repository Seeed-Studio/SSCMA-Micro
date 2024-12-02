#ifndef _MA_NMS_H_
#define _MA_NMS_H_

#include <algorithm>
#include <forward_list>
#include <iterator>

#include "../ma_types.h"

namespace ma::utils {

// skip use of template since it is not allowed

void nms(std::forward_list<ma_bbox_t>& bboxes, float threshold_iou, float threshold_score, bool soft_nms, bool multi_target);

void nms(std::forward_list<ma_bbox_ext_t>& bboxes, float threshold_iou, float threshold_score, bool soft_nms, bool multi_target);

void nms(std::forward_list<ma_keypoint3f_t>& decodings, const float iou_thr, bool should_nms_cross_classes);

}  // namespace ma::utils

#endif  // _MA_NMS_H_