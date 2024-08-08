#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <vector>

#include "core/math/ma_math_scalars.h"

namespace ma {

using namespace ma::math;

TEST(MATH, ScalarsFastLn) {
    std::vector<float> data = {
      -10, -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1, -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3, -0.2, -0.1, 0,
      0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1,  2,    3,    4,    5,    6,    7,    8,    9,    10,
    };
    std::vector<float> expected;
    for (const auto& d : data) {
        expected.push_back(std::log(d));
    }

    EXPECT_EQ(data.size(), expected.size());
    for (size_t i = 0; i < data.size(); ++i) {
        auto result = fastLn(data[i]);

        if (result != result) {
            EXPECT_TRUE(expected[i] != expected[i]);
        } else if (std::isinf(result)) {
            EXPECT_TRUE(std::isinf(expected[i]));
        } else {
            EXPECT_NEAR(result, expected[i], 1e-1);
        }
    }
}

TEST(MATH, ScalarsFastExp) {
    std::vector<float> data = {
      -3,  -2,  -1,  -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3, -0.2, -0.1, 0,   0.1, 0.2, 0.3,
      0.4, 0.5, 0.6, 0.7,  0.8,  0.9,  1,    1.1,  1.2,  1.3,  1.4,  1.5,  1.6, 1.7, 2,
    };
    std::vector<float> expected;
    for (const auto& d : data) {
        expected.push_back(std::exp(d));
    }

    EXPECT_EQ(data.size(), expected.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_NEAR(fastExp(data[i]), expected[i], 2e-1);
    }
}

TEST(MATH, ScalarsFastSigmoid) {
    std::vector<float> data = {
      -10, -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1, -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3, -0.2, -0.1, 0,
      0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1,  2,    3,    4,    5,    6,    7,    8,    9,    10,
    };
    std::vector<float> expected;
    for (const auto& d : data) {
        expected.push_back(sigmoid(d));
    }

    EXPECT_EQ(data.size(), expected.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_NEAR(fastSigmoid(data[i]), expected[i], 1e-1);
    }
}

TEST(MATH, ScalarsInverseSigmoid) {
    std::vector<float> expected = {
      -10, -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1, -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3, -0.2, -0.1, 0,
      0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1,  2,    3,    4,    5,    6,    7,    8,    9,    10,
    };
    std::vector<float> data;
    for (const auto& d : expected) {
        data.push_back(sigmoid(d));
    }

    EXPECT_EQ(data.size(), expected.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_NEAR(inverseSigmoid(data[i]), expected[i], 1e-2);
    }
}

TEST(MATH, ScalarsQuantizeValue) {
    std::vector<float> data = {
      -10, -9,  -8,  -7,  -6,  -5,  -4,  -3,  -2,  -1, -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3, -0.2, -0.1, 0,
      0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1,  2,    3,    4,    5,    6,    7,    8,    9,    10,
    };
    std::vector<int32_t> expected;
    for (const auto& d : data) {
        expected.push_back(static_cast<int32_t>(std::round(d / 0.1f) + 0));
    }

    EXPECT_EQ(data.size(), expected.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(quantizeValue(data[i], 0.1f, 0), expected[i]);
    }
}

TEST(MATH, ScalarsDequantizeValue) {
    std::vector<int32_t> data = {
      -10, -9, -8, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    };
    std::vector<float> expected;
    for (const auto& d : data) {
        expected.push_back(static_cast<float>(d - 0) * 0.1f);
    }

    EXPECT_EQ(data.size(), expected.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(dequantizeValue(data[i], 0.1f, 0), expected[i]);
    }
}

}  // namespace ma
