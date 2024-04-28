/*
 * MIT License
 * Copyright (c) 2021 Yifu Zhang
 *
 * Modified by nullptr, Apr 15, 2024, Seeed Technology Co.,Ltd
*/

#pragma once

#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Dense>

typedef Eigen::Matrix<float, 1, 4, Eigen::RowMajor>                DETECTBOX;
typedef Eigen::Matrix<float, -1, 4, Eigen::RowMajor>               DETECTBOXSS;
typedef Eigen::Matrix<float, 1, 128, Eigen::RowMajor>              FEATURE;
typedef Eigen::Matrix<float, Eigen::Dynamic, 128, Eigen::RowMajor> FEATURESS;

//Kalmanfilter
typedef Eigen::Matrix<float, 1, 8, Eigen::RowMajor> KAL_MEAN;
typedef Eigen::Matrix<float, 8, 8, Eigen::RowMajor> KAL_COVA;
typedef Eigen::Matrix<float, 1, 4, Eigen::RowMajor> KAL_HMEAN;
typedef Eigen::Matrix<float, 4, 4, Eigen::RowMajor> KAL_HCOVA;
using KAL_DATA  = std::pair<KAL_MEAN, KAL_COVA>;
using KAL_HDATA = std::pair<KAL_HMEAN, KAL_HCOVA>;

//main
using RESULT_DATA = std::pair<int, DETECTBOX>;

//tracker
using TRACKER_DATA = std::pair<int, FEATURESS>;
using MATCH_DATA   = std::pair<int, int>;

//linear_assignment
typedef Eigen::Matrix<float, -1, -1, Eigen::RowMajor> DYNAMICM;

struct Rect4f {
    float x;
    float y;
    float width;
    float height;
};

struct Scalar3u {
    Scalar3u(unsigned int v1, unsigned int v2, unsigned int v3) : val1(v1), val2(v2), val3(v3) {}

    unsigned int val1;
    unsigned int val2;
    unsigned int val3;
};
