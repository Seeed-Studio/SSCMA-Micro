#include "ma_model_detector.h"

namespace ma::model {

constexpr char TAG[] = "ma::model::detecor";

PoseDetector::PoseDetector(Engine* p_engine, const char* name, ma_model_type_t type) : Detector(p_engine, name, type){};

const std::vector<ma_keypoint3f_t>& PoseDetector::getResults() const { return results_; }

PoseDetector::~PoseDetector() {}

}  // namespace ma::model
