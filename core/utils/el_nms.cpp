/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu (Seeed Technology Inc.)
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

#include "el_nms.h"

#include <algorithm>

#include "core/el_compiler.h"

namespace edgelab {

bool box_comparator_sort(const el_box_t& box1, const el_box_t& box2) { return box1.score > box2.score; }

uint8_t compute_iou(const el_box_t& box1, const el_box_t& box2) {
    float x1    = std::max(box1.x, box2.x);
    float y1    = std::max(box1.y, box2.y);
    float x2    = std::min(box1.x + box1.w, box2.x + box2.w);
    float y2    = std::min(box1.y + box1.h, box2.y + box2.h);
    float w     = std::max(0.0f, x2 - x1);
    float h     = std::max(0.0f, y2 - y1);
    float inter = w * h;
    float iou   = inter / (box1.w * box1.h + box2.w * box2.h - inter);
    return iou * 100;
}

EL_ATTR_WEAK int el_nms(std::forward_list<el_box_t>& boxes,
                        uint8_t                      nms_iou_thresh,
                        uint8_t                      nms_score_thresh,
                        bool                         soft_nms,
                        bool                         multi_target) {
    boxes.sort(box_comparator_sort);

    for (auto it = boxes.begin(); it != boxes.end(); it++) {
        if (it->score == 0) continue;
        for (auto it2 = std::next(it); it2 != boxes.end(); it2++) {
            if (it2->score == 0) continue;
            if (multi_target && it->target != it2->target) continue;
            uint8_t iou = compute_iou(*it, *it2);
            if (iou > nms_iou_thresh) {
                if (soft_nms) {  // soft-nms
                    it2->score = it2->score * (1 - iou / 100.0f);
                    if (it2->score < nms_score_thresh) it2->score = 0;
                } else {
                    it2->score = 0;
                }
            }
        }
    }
    boxes.remove_if([](const el_box_t& box) { return box.score == 0; });
    return std::distance(boxes.begin(), boxes.end());
}

}  // namespace edgelab
