#ifndef _MA_UTILS_ANCHORS_H_
#define _MA_UTILS_ANCHORS_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../ma_types.h"

#if MA_USE_LIB_XTENSOR
#include <xtensor/xarray.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xbuilder.hpp>
#endif

namespace ma::utils {

std::vector<ma_anchor_stride_t> generateAnchorStrides(size_t input_size, std::vector<size_t> strides = {8, 16, 32});

std::vector<std::vector<ma_pt2f_t>> generateAnchorMatrix(const std::vector<ma_anchor_stride_t>& anchor_strides, float shift_right = 1.f, float shift_down = 1.f);

#if MA_USE_LIB_XTENSOR
std::vector<xt::xarray<double>> generateAnchorMatrix(std::vector<int>& strides, std::vector<int> network_dims, std::size_t boxes_num, int strided_width, int strided_height);
#endif

}  // namespace ma::utils

#endif