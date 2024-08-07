#ifndef _MA_UTILS_ANCHORS_H_
#define _MA_UTILS_ANCHORS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "ma_types.h"

namespace ma::utils {

std::vector<ma_anchor_stride_t> generateAnchorStrides(size_t input_size, std::vector<size_t> strides);

std::vector<std::vector<ma_pt2f_t>> generateAnchorMatrix(const std::vector<ma_anchor_stride_t>& anchor_strides,
                                                         float                                  shift_right,
                                                         float                                  shift_down);

}  // namespace ma::utils

#endif