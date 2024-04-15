/*
 * MIT License
 * Copyright (c) 2021 Yifu Zhang
 *
 * Modified by nullptr, Apr 15, 2024, Seeed Technology Co.,Ltd
*/

#include <cassert>
#include <cfloat>
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <new>
#include <vector>

#include "BYTETracker.h"
#include "lapjv.h"

using namespace std;

vector<STrack*> BYTETracker::joint_stracks(vector<STrack*>& tlista, vector<STrack>& tlistb) {
    map<int, int>   exists;
    vector<STrack*> res;
    for (int i = 0; i < tlista.size(); i++) {
        exists.insert(pair<int, int>(tlista[i]->track_id, 1));
        res.push_back(tlista[i]);
    }
    for (int i = 0; i < tlistb.size(); i++) {
        int tid = tlistb[i].track_id;
        if (!exists[tid] || exists.count(tid) == 0) {
            exists[tid] = 1;
            res.push_back(&tlistb[i]);
        }
    }
    return res;
}

vector<STrack> BYTETracker::joint_stracks(vector<STrack>& tlista, vector<STrack>& tlistb) {
    map<int, int>  exists;
    vector<STrack> res;
    for (int i = 0; i < tlista.size(); i++) {
        exists.insert(pair<int, int>(tlista[i].track_id, 1));
        res.push_back(tlista[i]);
    }
    for (int i = 0; i < tlistb.size(); i++) {
        int tid = tlistb[i].track_id;
        if (!exists[tid] || exists.count(tid) == 0) {
            exists[tid] = 1;
            res.push_back(tlistb[i]);
        }
    }
    return res;
}

vector<STrack> BYTETracker::sub_stracks(vector<STrack>& tlista, vector<STrack>& tlistb) {
    map<int, STrack> stracks;
    for (int i = 0; i < tlista.size(); i++) {
        stracks.insert(pair<int, STrack>(tlista[i].track_id, tlista[i]));
    }
    for (int i = 0; i < tlistb.size(); i++) {
        int tid = tlistb[i].track_id;
        if (stracks.count(tid) != 0) {
            stracks.erase(tid);
        }
    }

    vector<STrack>                  res;
    std::map<int, STrack>::iterator it;
    for (it = stracks.begin(); it != stracks.end(); ++it) {
        res.push_back(it->second);
    }

    return res;
}

void BYTETracker::remove_duplicate_stracks(vector<STrack>& resa,
                                           vector<STrack>& resb,
                                           vector<STrack>& stracksa,
                                           vector<STrack>& stracksb) {
    vector<vector<float> >  pdist = iou_distance(stracksa, stracksb);
    vector<pair<int, int> > pairs;
    for (int i = 0; i < pdist.size(); i++) {
        for (int j = 0; j < pdist[i].size(); j++) {
            if (pdist[i][j] < 0.15) {
                pairs.push_back(pair<int, int>(i, j));
            }
        }
    }

    vector<int> dupa, dupb;
    for (int i = 0; i < pairs.size(); i++) {
        int timep = stracksa[pairs[i].first].frame_id - stracksa[pairs[i].first].start_frame;
        int timeq = stracksb[pairs[i].second].frame_id - stracksb[pairs[i].second].start_frame;
        if (timep > timeq)
            dupb.push_back(pairs[i].second);
        else
            dupa.push_back(pairs[i].first);
    }

    for (int i = 0; i < stracksa.size(); i++) {
        vector<int>::iterator iter = find(dupa.begin(), dupa.end(), i);
        if (iter == dupa.end()) {
            resa.push_back(stracksa[i]);
        }
    }

    for (int i = 0; i < stracksb.size(); i++) {
        vector<int>::iterator iter = find(dupb.begin(), dupb.end(), i);
        if (iter == dupb.end()) {
            resb.push_back(stracksb[i]);
        }
    }
}

void BYTETracker::linear_assignment(vector<vector<float> >& cost_matrix,
                                    int                     cost_matrix_size,
                                    int                     cost_matrix_size_size,
                                    float                   thresh,
                                    vector<vector<int> >&   matches,
                                    vector<int>&            unmatched_a,
                                    vector<int>&            unmatched_b) {
    if (cost_matrix.size() == 0) {
        for (int i = 0; i < cost_matrix_size; i++) {
            unmatched_a.push_back(i);
        }
        for (int i = 0; i < cost_matrix_size_size; i++) {
            unmatched_b.push_back(i);
        }
        return;
    }

    vector<int> rowsol;
    vector<int> colsol;

    lapjv(cost_matrix, rowsol, colsol, true, thresh);

    auto rowsol_size = rowsol.size();
    for (int i = 0; i < rowsol_size; i++) {
        if (rowsol[i] >= 0) {
            matches.emplace_back(vector<int>{i, rowsol[i]});
        } else {
            unmatched_a.push_back(i);
        }
    }

    auto colsol_size = colsol.size();
    for (int i = 0; i < colsol_size; i++) {
        if (colsol[i] < 0) {
            unmatched_b.push_back(i);
        }
    }
}

vector<vector<float> > BYTETracker::ious(vector<vector<float> >& atlbrs, vector<vector<float> >& btlbrs) {
    vector<vector<float> > ious;
    if (atlbrs.size() * btlbrs.size() == 0) return ious;

    ious.resize(atlbrs.size());
    auto ious_size = ious.size();
    for (int i = 0; i < ious_size; i++) {
        ious[i].resize(btlbrs.size());
    }

    //bbox_ious
    auto btlbrs_size = btlbrs.size();
    auto atlbrs_size = atlbrs.size();
    for (int k = 0; k < btlbrs_size; k++) {
        float box_area = (btlbrs[k][2] - btlbrs[k][0] + 1) * (btlbrs[k][3] - btlbrs[k][1] + 1);
        for (int n = 0; n < atlbrs_size; n++) {
            float iw = min(atlbrs[n][2], btlbrs[k][2]) - max(atlbrs[n][0], btlbrs[k][0]) + 1;
            if (iw > 0) {
                float ih = min(atlbrs[n][3], btlbrs[k][3]) - max(atlbrs[n][1], btlbrs[k][1]) + 1;
                if (ih > 0) {
                    float ua =
                      (atlbrs[n][2] - atlbrs[n][0] + 1) * (atlbrs[n][3] - atlbrs[n][1] + 1) + box_area - iw * ih;
                    ious[n][k] = iw * ih / ua;
                } else {
                    ious[n][k] = 0.0;
                }
            } else {
                ious[n][k] = 0.0;
            }
        }
    }

    return ious;
}

vector<vector<float> > BYTETracker::iou_distance(vector<STrack*>& atracks,
                                                 vector<STrack>&  btracks,
                                                 int&             dist_size,
                                                 int&             dist_size_size) {
    vector<vector<float> > cost_matrix;
    if (atracks.size() * btracks.size() == 0) {
        dist_size      = atracks.size();
        dist_size_size = btracks.size();
        return cost_matrix;
    }

    auto atracks_size = atracks.size();
    auto btracks_size = btracks.size();

    vector<vector<float> > atlbrs, btlbrs;
    atlbrs.resize(atracks_size);
    btlbrs.resize(btracks_size);
    for (int i = 0; i < atracks_size; i++) {
        atlbrs[i] = atracks[i]->tlbr;
    }
    for (int i = 0; i < btracks_size; i++) {
        btlbrs[i] = btracks[i].tlbr;
    }

    dist_size      = atracks.size();
    dist_size_size = btracks.size();

    vector<vector<float> > _ious{ious(atlbrs, btlbrs)};
    auto                   _ious_size = _ious.size();
    cost_matrix.resize(_ious_size);

    for (int i = 0; i < _ious_size; i++) {
        vector<float> _iou;
        auto          _ious_i_size = _ious[i].size();
        _iou.resize(_ious_i_size);
        for (int j = 0; j < _ious_i_size; j++) {
            _iou[j] = 1 - _ious[i][j];
        }
        cost_matrix[i] = move(_iou);
    }

    return cost_matrix;
}

vector<vector<float> > BYTETracker::iou_distance(vector<STrack>& atracks, vector<STrack>& btracks) {
    auto atracks_size = atracks.size();
    auto btracks_size = btracks.size();

    static vector<vector<float> > atlbrs, btlbrs;

    atlbrs.resize(atracks_size);
    btlbrs.resize(btracks_size);
    for (int i = 0; i < atracks_size; i++) {
        atlbrs[i] = atracks[i].tlbr;
    }
    for (int i = 0; i < btracks_size; i++) {
        btlbrs[i] = btracks[i].tlbr;
    }

    vector<vector<float> > _ious{ious(atlbrs, btlbrs)};
    auto                   _ious_size = _ious.size();
    vector<vector<float> > cost_matrix;
    cost_matrix.resize(_ious_size);

    for (int i = 0; i < _ious_size; i++) {
        vector<float> _iou;
        auto          _ious_i_size = _ious[i].size();
        _iou.resize(_ious_i_size);
        for (int j = 0; j < _ious_i_size; j++) {
            _iou[j] = 1 - _ious[i][j];
        }
        cost_matrix[i] = move(_iou);
    }

    return cost_matrix;
}

double BYTETracker::lapjv(const vector<vector<float> >& cost,
                          vector<int>&                  rowsol,
                          vector<int>&                  colsol,
                          bool                          extend_cost,
                          float                         cost_limit,
                          bool                          return_cost) {
    vector<vector<float> > cost_c;
    cost_c.assign(cost.begin(), cost.end());

    vector<vector<float> > cost_c_extended;

    int n_rows = cost.size();
    int n_cols = cost[0].size();
    rowsol.resize(n_rows);
    colsol.resize(n_cols);

    int n = 0;
    if (n_rows == n_cols) {
        n = n_rows;
    } else {
        if (!extend_cost) {
            assert(0 && "set extend_cost=True");
        }
    }

    if (extend_cost || cost_limit < LONG_MAX) {
        n = n_rows + n_cols;
        cost_c_extended.resize(n);
        for (int i = 0; i < n; i++) cost_c_extended[i].resize(n);

        if (cost_limit < LONG_MAX) {
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < cost_c_extended[i].size(); j++) {
                    cost_c_extended[i][j] = cost_limit / 2.0;
                }
            }
        } else {
            float cost_max = -1.0;
            for (int i = 0; i < cost_c.size(); i++) {
                for (int j = 0; j < cost_c[i].size(); j++) {
                    if (cost_c[i][j] > cost_max) cost_max = cost_c[i][j];
                }
            }
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < cost_c_extended[i].size(); j++) {
                    cost_c_extended[i][j] = cost_max + 1;
                }
            }
        }

        for (int i = n_rows; i < n; i++) {
            for (int j = n_cols; j < cost_c_extended[i].size(); j++) {
                cost_c_extended[i][j] = 0;
            }
        }
        for (int i = 0; i < n_rows; i++) {
            for (int j = 0; j < n_cols; j++) {
                cost_c_extended[i][j] = cost_c[i][j];
            }
        }

        cost_c.clear();
        cost_c.swap(cost_c_extended);
    }

    double** cost_ptr;
    cost_ptr = new double*[sizeof(double*) * n];
    for (int i = 0; i < n; i++) cost_ptr[i] = new double[sizeof(double) * n];

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            cost_ptr[i][j] = cost_c[i][j];
        }
    }

    int* x_c = new int[sizeof(int) * n];
    int* y_c = new int[sizeof(int) * n];

    double opt = 0.0;

    int ret = lapjv_internal(n, cost_ptr, x_c, y_c);
    if (ret != 0) {
        puts("lapjv_internal failed");
        goto ReleaseMemory;
    }

    if (n != n_rows) {
        for (int i = 0; i < n; i++) {
            if (x_c[i] >= n_cols) x_c[i] = -1;
            if (y_c[i] >= n_rows) y_c[i] = -1;
        }
        for (int i = 0; i < n_rows; i++) {
            rowsol[i] = x_c[i];
        }
        for (int i = 0; i < n_cols; i++) {
            colsol[i] = y_c[i];
        }

        if (return_cost) {
            for (int i = 0; i < rowsol.size(); i++) {
                if (rowsol[i] != -1) {
                    opt += cost_ptr[i][rowsol[i]];
                }
            }
        }
    } else if (return_cost) {
        for (int i = 0; i < rowsol.size(); i++) {
            opt += cost_ptr[i][rowsol[i]];
        }
    }

ReleaseMemory:
    for (int i = 0; i < n; i++) {
        delete[] cost_ptr[i];
    }
    delete[] cost_ptr;
    delete[] x_c;
    delete[] y_c;

    return opt;
}
