#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <vector>

#include "Eigen/Dense"

namespace ma {

TEST(MISC, MiscCheckEigenInstall) {
    using Eigen::MatrixXd;

    MatrixXd m(2, 2);

    m(0, 0) = 3;
    m(1, 0) = 2.5;
    m(0, 1) = -1;
    m(1, 1) = m(1, 0) + m(0, 1);

    EXPECT_EQ(m(0, 0), 3);
    EXPECT_EQ(m(1, 0), 2.5);
    EXPECT_EQ(m(0, 1), -1);
    EXPECT_EQ(m(1, 1), 1.5);
}

}  // namespace ma