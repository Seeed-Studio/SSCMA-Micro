#ifndef _MA_MODEL_POSE_DETECTOR_H_
#define _MA_MODEL_POSE_DETECTOR_H_

#include <vector>

#include "ma_model_detector.h"

namespace ma::model {

class PoseDetector : public Detector {
   protected:
    std::vector<ma_keypoint3f_t> results_;

   public:
    PoseDetector(Engine* engine, const char* name, ma_model_type_t type);
    virtual ~PoseDetector();

    const std::vector<ma_keypoint3f_t>& getResults() const;
};

}  // namespace ma::model

#endif  // _MA_MODEL_POSE_DETECTOR_H
