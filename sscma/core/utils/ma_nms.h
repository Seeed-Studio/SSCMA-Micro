#ifndef _MA_NMS_H_
#define _MA_NMS_H_

#include "core/ma_common.h"
#include <forward_list>

namespace ma::utils {

#ifdef __cplusplus
extern "C" {
#endif


int nms(std::forward_list<ma_bbox_t>& boxes,
        float threshold_iou,
        float threshold_score,
        bool soft_nms     = false,
        bool multi_target = false);
}

}  // namespace ma::utils

#endif  // _MA_NMS_H_