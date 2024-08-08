/*
 * MIT License
 * Copyright (c) 2021 Yifu Zhang
 *
 * Modified by nullptr, Aug 8, 2024, Seeed Technology Co.,Ltd
*/

#ifndef _BYTETRACK_KALMAN_FILTER_H_
#define _BYTETRACK_KALMAN_FILTER_H_

#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "Eigen/Cholesky"
#include "Eigen/Dense"

typedef Eigen::Matrix<float, 1, 4, Eigen::RowMajor>                DETECTBOX;
typedef Eigen::Matrix<float, -1, 4, Eigen::RowMajor>               DETECTBOXSS;
typedef Eigen::Matrix<float, 1, 128, Eigen::RowMajor>              FEATURE;
typedef Eigen::Matrix<float, Eigen::Dynamic, 128, Eigen::RowMajor> FEATURESS;

typedef Eigen::Matrix<float, 1, 8, Eigen::RowMajor> KAL_MEAN;
typedef Eigen::Matrix<float, 8, 8, Eigen::RowMajor> KAL_COVA;
typedef Eigen::Matrix<float, 1, 4, Eigen::RowMajor> KAL_HMEAN;
typedef Eigen::Matrix<float, 4, 4, Eigen::RowMajor> KAL_HCOVA;

using KAL_DATA  = std::pair<KAL_MEAN, KAL_COVA>;
using KAL_HDATA = std::pair<KAL_HMEAN, KAL_HCOVA>;

using RESULT_DATA = std::pair<int, DETECTBOX>;

using TRACKER_DATA = std::pair<int, FEATURESS>;
using MATCH_DATA   = std::pair<int, int>;

typedef Eigen::Matrix<float, -1, -1, Eigen::RowMajor> DYNAMICM;

class KalmanFilter {
   public:
    KalmanFilter();

    KAL_DATA  initiate(const DETECTBOX& measurement);
    void      predict(KAL_MEAN& mean, KAL_COVA& covariance);
    KAL_HDATA project(const KAL_MEAN& mean, const KAL_COVA& covariance);
    KAL_DATA  update(const KAL_MEAN& mean, const KAL_COVA& covariance, const DETECTBOX& measurement);

   private:
    Eigen::Matrix<float, 8, 8, Eigen::RowMajor> _motion_mat;
    Eigen::Matrix<float, 4, 8, Eigen::RowMajor> _update_mat;

    float _std_weight_position;
    float _std_weight_velocity;
};

#endif