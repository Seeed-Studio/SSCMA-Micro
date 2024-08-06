#ifndef _MA_MATH_VECTORS_H
#define _MA_MATH_VECTORS_H

#include <cstdint>
#include <cstddef>

namespace ma::math {

void softMax(float* data, size_t size);

void fastSoftMax(float* data, size_t size);

}  // namespace ma::math

#endif