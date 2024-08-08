/*
 * MIT License
 * Copyright (c) 2021 Yifu Zhang
 *
 * Modified by nullptr, Aug 8, 2024, Seeed Technology Co.,Ltd
*/

#ifndef _BYTETRACK_STRACK_H_
#define _BYTETRACK_STRACK_H_

#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "kalman_filter.h"

enum TrackState { New = 0, Tracked, Lost, Removed };

class STrack {
   public:
    STrack(std::vector<float> tlwh_, float score, int label);
    ~STrack();

    std::vector<float> static tlbr_to_tlwh(std::vector<float>& tlbr);
    void static multi_predict(std::vector<STrack*>& stracks, KalmanFilter& kalman_filter);
    void               static_tlwh();
    void               static_tlbr();
    std::vector<float> tlwh_to_xyah(std::vector<float> tlwh_tmp);
    std::vector<float> to_xyah();
    void               mark_lost();
    void               mark_removed();
    int                next_id();
    int                end_frame();

    void activate(KalmanFilter& kalman_filter, int frame_id);
    void re_activate(STrack& new_track, int frame_id, bool new_id = false);
    void update(STrack& new_track, int frame_id);

   public:
    bool is_activated;
    int  track_id;
    int  state;

    std::vector<float> _tlwh;
    std::vector<float> tlwh;
    std::vector<float> tlbr;

    int frame_id;
    int tracklet_len;
    int start_frame;

    KAL_MEAN mean;
    KAL_COVA covariance;

    float score;
    int   label;

   private:
    KalmanFilter kalman_filter;
};

#endif