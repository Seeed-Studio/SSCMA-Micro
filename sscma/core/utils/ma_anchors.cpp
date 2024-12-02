#include "ma_anchors.h"

namespace ma::utils {

std::vector<ma_anchor_stride_t> generateAnchorStrides(size_t input_size, std::vector<size_t> strides) {
    std::vector<ma_anchor_stride_t> anchor_strides(strides.size());
    size_t nth_anchor = 0;

    for (size_t i = 0; i < strides.size(); ++i) {
        const size_t stride = strides[i];
        const size_t split  = input_size / stride;
        const size_t size   = split * split;
        anchor_strides[i]   = {stride, split, size, nth_anchor};
        nth_anchor += size;
    }

    return anchor_strides;
}

std::vector<std::vector<ma_pt2f_t>> generateAnchorMatrix(const std::vector<ma_anchor_stride_t>& anchor_strides, float shift_right, float shift_down) {
    const auto anchor_matrix_size = anchor_strides.size();
    std::vector<std::vector<ma_pt2f_t>> anchor_matrix(anchor_matrix_size);
    const float shift_right_init = shift_right * 0.5f;
    const float shift_down_init  = shift_down * 0.5f;

    for (size_t i = 0; i < anchor_matrix_size; ++i) {
        const auto& anchor_stride = anchor_strides[i];
        const auto split          = anchor_stride.split;
        const auto size           = anchor_stride.size;
        auto& anchor_matrix_i     = anchor_matrix[i];

        anchor_matrix[i].resize(size);

        for (size_t j = 0; j < size; ++j) {
            const float x      = static_cast<float>(j % split) * shift_right + shift_right_init;
            const float y      = static_cast<float>(j / split) * shift_down + shift_down_init;
            anchor_matrix_i[j] = {x, y};
        }
    }

    return anchor_matrix;
}

#if MA_USE_LIB_XTENSOR

/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

std::vector<xt::xarray<double>> generateAnchorMatrix(std::vector<int>& strides, std::vector<int> network_dims, std::size_t boxes_num, int strided_width, int strided_height) {
    std::vector<xt::xarray<double>> centers(boxes_num);

    for (uint i = 0; i < boxes_num; ++i) {
        strided_width  = network_dims[0] / strides[i];
        strided_height = network_dims[1] / strides[i];

        // Create a meshgrid of the proper strides
        xt::xarray<int> grid_x = xt::arange(0, strided_width);
        xt::xarray<int> grid_y = xt::arange(0, strided_height);

        auto mesh = xt::meshgrid(grid_x, grid_y);
        grid_x    = std::get<1>(mesh);
        grid_y    = std::get<0>(mesh);

        // Use the meshgrid to build up box center prototypes
        auto ct_row = (xt::flatten(grid_y) + 0.5) * strides[i];
        auto ct_col = (xt::flatten(grid_x) + 0.5) * strides[i];

        centers[i] = xt::stack(xt::xtuple(ct_col, ct_row, ct_col, ct_row), 1);
    }

    return centers;
}

#endif

}  // namespace ma::utils
