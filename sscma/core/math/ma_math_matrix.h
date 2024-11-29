#ifndef _MA_MATH_MARTRIX_H_
#define _MA_MATH_MARTRIX_H_

#include <cstddef>
#include <cstdint>

#if MA_USE_LIB_XTENSOR
#include <xtensor/xarray.hpp>
#include <xtensor/xmath.hpp>
#include <xtensor/xview.hpp>
#endif

namespace ma::math {

void softmax2D(float* data, size_t rows, size_t cols);

void fastSoftmax2D(float* data, size_t rows, size_t cols);

#if MA_USE_LIB_XTENSOR
template <typename QT>
static void dequantizeValues2D(xt::xarray<float>& dequantized_outputs, int index, const xt::xarray<QT>& quantized_outputs, size_t dim1, size_t dim2, float32_t qp_scale, float32_t qp_zp) {
    for (size_t i = 0; i < dim1; i++) {
        for (size_t j = 0; j < dim2; j++) {
            dequantized_outputs(i, j) = (float(quantized_outputs(index, i, j)) - qp_zp) * qp_scale;
        }
    }
}
#endif

}  // namespace ma::math

#endif