#ifndef _MA_MATH_VECTORS_H_
#define _MA_MATH_VECTORS_H_

#include <cstddef>
#include <cstdint>

#if MA_USE_LIB_XTENSOR
#include <xtensor/xarray.hpp>
#include <xtensor/xmath.hpp>
#include <xtensor/xview.hpp>
#endif

namespace ma::math {

void softmax(float* data, size_t size);

void fastSoftmax(float* data, size_t size);

#if MA_USE_LIB_XTENSOR
template <typename QT>
static void dequantizeValues1D(xt::xarray<float>& dequantized_outputs, int index, const xt::xarray<QT>& quantized_outputs, size_t dim1, float32_t qp_scale, float32_t qp_zp) {
    for (size_t i = 0; i < dim1; ++i) {
        dequantized_outputs(i) = (float(quantized_outputs(index, i)) - qp_zp) * qp_scale;
    }
}
#endif

}  // namespace ma::math

#endif