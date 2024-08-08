#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <vector>

#include "core/math/ma_math_vectors.h"

namespace ma {

using namespace ma::math;

TEST(MATH, VectorsSoftmax) {
    std::vector<float> data = {
      -1.,        -0.42105263, 0.15789474, 0.73684211, 1.31578947, 1.89473684, 2.47368421,
      3.05263158, 3.63157895,  4.21052632, 4.78947368, 5.36842105, 5.94736842, 6.52631579,
      7.10526316, 7.68421053,  8.26315789, 8.84210526, 9.42105263, 10.,
    };
    std::vector<float> expected = {
      7.3407e-06, 1.3097e-05, 2.3367e-05, 4.1690e-05, 7.4382e-05, 1.3271e-04, 2.3678e-04,
      4.2245e-04, 7.5371e-04, 1.3447e-03, 2.3992e-03, 4.2806e-03, 7.6373e-03, 1.3626e-02,
      2.4311e-02, 4.3375e-02, 7.7388e-02, 1.3807e-01, 2.4634e-01, 4.3952e-01,
    };

    EXPECT_EQ(data.size(), expected.size());

    auto copy = data;
    EXPECT_EQ(data.size(), copy.size());

    softmax(copy.data(), copy.size());

    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_NEAR(copy[i], expected[i], 1e-3);
    }
}

TEST(MATH, VectorsFastSoftmax) {
    std::vector<float> data = {
      -1.,        -0.42105263, 0.15789474, 0.73684211, 1.31578947, 1.89473684, 2.47368421,
      3.05263158, 3.63157895,  4.21052632, 4.78947368, 5.36842105, 5.94736842, 6.52631579,
      7.10526316, 7.68421053,  8.26315789, 8.84210526, 9.42105263, 10.,
    };
    std::vector<float> expected = {
      7.3407e-06, 1.3097e-05, 2.3367e-05, 4.1690e-05, 7.4382e-05, 1.3271e-04, 2.3678e-04,
      4.2245e-04, 7.5371e-04, 1.3447e-03, 2.3992e-03, 4.2806e-03, 7.6373e-03, 1.3626e-02,
      2.4311e-02, 4.3375e-02, 7.7388e-02, 1.3807e-01, 2.4634e-01, 4.3952e-01,
    };

    EXPECT_EQ(data.size(), expected.size());

    auto copy = data;
    EXPECT_EQ(data.size(), copy.size());

    fastSoftmax(copy.data(), copy.size());

    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_NEAR(copy[i], expected[i], 1e-2);
    }
}

}  // namespace ma