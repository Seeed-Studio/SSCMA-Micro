
#include <cmath>

#include "ma_nms.h"

namespace ma::utils {

bool box_comparator_sort(const ma_bbox_t& box1, const ma_bbox_t& box2) {
    return box1.score > box2.score;
}

uint8_t compute_iou(const ma_bbox_t& box1, const ma_bbox_t& box2) {
    float x1    = std::max(box1.x, box2.x);
    float y1    = std::max(box1.y, box2.y);
    float x2    = std::min(box1.x + box1.w, box2.x + box2.w);
    float y2    = std::min(box1.y + box1.h, box2.y + box2.h);
    float w     = std::max(0.0f, x2 - x1);
    float h     = std::max(0.0f, y2 - y1);
    float inter = w * h;
    float iou   = inter / (box1.w * box1.h + box2.w * box2.h - inter);
    return std::round(iou * 100.f);
}


int nms(std::forward_list<ma_bbox_t>& boxes,
        float                         threshold_iou,
        float                         threshold_score,
        bool                          soft_nms,
        bool                          multi_target) {
    boxes.sort(box_comparator_sort);

    for (auto it = boxes.begin(); it != boxes.end(); it++) {
        if (it->score == 0)
            continue;
        for (auto it2 = std::next(it); it2 != boxes.end(); it2++) {
            if (it2->score == 0)
                continue;
            if (multi_target && it->target != it2->target)
                continue;
            uint8_t iou = compute_iou(*it, *it2);
            if (iou > threshold_iou) {
                if (soft_nms) {  // soft-nms
                    it2->score = it2->score * (1 - iou / 100.0f);
                    if (it2->score < threshold_score)
                        it2->score = 0;
                } else {
                    it2->score = 0;
                }
            }
        }
    }
    boxes.remove_if([](const ma_bbox_t& box) { return box.score == 0; });
    return std::distance(boxes.begin(), boxes.end());
}

}