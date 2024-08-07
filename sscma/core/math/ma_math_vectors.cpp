
#include "ma_math_vectors.h"

#include <climits>
#include <cmath>

#include "ma_math_scalars.h"

namespace ma::math {

#ifdef MA_MATH_FAST_SOFTMAX_IMPL
    #undef MA_MATH_FAST_SOFTMAX_IMPL
#endif

#define MA_MATH_FAST_SOFTMAX_IMPL(data, size, exp_f)                 \
    {                                                                \
        if (!data) [[unlikely]] {                                    \
            return;                                                  \
        }                                                            \
                                                                     \
        float sum{0.f};                                              \
                                                                     \
        for (size_t i = 0; i < size; ++i) {                          \
            auto&      data_i = data[i];                             \
            const auto exp_i  = exp_f(data_i);                       \
            sum += exp_i;                                            \
            data_i = exp_i;                                          \
        }                                                            \
                                                                     \
        if (std::abs(sum) < std::numeric_limits<float>::epsilon()) { \
            sum = std::numeric_limits<float>::epsilon();             \
        }                                                            \
                                                                     \
        for (size_t i = 0; i < size; ++i) {                          \
            data[i] /= sum;                                          \
        }                                                            \
    }

void softmax(float* data, size_t size) { MA_MATH_FAST_SOFTMAX_IMPL(data, size, std::exp); }

void fastSoftmax(float* data, size_t size) { MA_MATH_FAST_SOFTMAX_IMPL(data, size, fastExp); }

}  // namespace ma::math
