
#ifndef _MA_MATH_SCALARS_H_
#define _MA_MATH_SCALARS_H_

#include <cmath>
#include <cstdint>
#include <limits>

namespace ma::math {

constexpr inline float fastLn(float x) {
    // "Remez approximation of ln(x)""
    // https://sites.tufts.edu/atasissa/files/2019/09/remez.pdf

    static_assert(sizeof(unsigned int) == sizeof(float));
    static_assert(sizeof(float) == 4);

    if (*reinterpret_cast<unsigned int*>(&x) & 0x80000000) [[unlikely]] {
        return -std::numeric_limits<float>::quiet_NaN();
    } else if (!((*reinterpret_cast<unsigned int*>(&x) & 0x7FFFFFFF) ^ 0x00000000)) [[unlikely]] {
        return -std::numeric_limits<float>::infinity();
    }

    auto bx{*reinterpret_cast<unsigned int*>(&x)};
    auto ex{bx >> 23};
    const auto t{static_cast<signed int>(ex) - static_cast<signed int>(127)};

    bx = 1065353216 | (bx & 8388607);
    x  = *reinterpret_cast<float*>(&bx);
    return static_cast<float>(-1.49278 + (2.11263 + (-0.729104 + 0.10969 * x) * x) * x + 0.6931471806 * t);
}

constexpr inline float fastExp(float x) {
    // N. Schraudolph, "A Fast, Compact Approximation of the Exponential Function"
    // https://nic.schraudolph.org/pubs/Schraudolph99.pdf

    static_assert(sizeof(float) == 4);

    x = (12102203.1608621 * x) + 1064986823.01029;

    const float c{8388608.f};
    const float d{2139095040.f};

    if ((x < c) | (x > d))
        x = (x < c) ? 0.0f : d;

    uint32_t n = static_cast<uint32_t>(x);
    x          = *reinterpret_cast<float*>(&n);

    return x;
}

constexpr inline float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

constexpr inline float fastSigmoid(float x) {
    return 1.0f / (1.0f + fastExp(-x));
}

constexpr inline float inverseSigmoid(float x) {
    float denominator = 1.0f - x;

    if (std::abs(denominator) < std::numeric_limits<float>::epsilon()) {
        denominator = std::numeric_limits<float>::epsilon();
    }

    return std::log(x / denominator);
}

constexpr inline int32_t quantizeValue(float value, float scale, int32_t zero_point) {
    return static_cast<int32_t>(std::round(value / scale) + zero_point);
}

constexpr inline int32_t quantizeValueFloor(float value, float scale, int32_t zero_point) {
    return static_cast<int32_t>(std::floor(value / scale) + zero_point);
}

constexpr inline float dequantizeValue(int32_t value, float scale, int32_t zero_point) {
    return static_cast<float>(value - zero_point) * scale;
}

}  // namespace ma::math

#endif  // _MA_MATH_SCALAR_H
