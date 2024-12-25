#ifndef _MA_MODEL_FACTORY_H_
#define _MA_MODEL_FACTORY_H_

#include "../ma_common.h"

#include "ma_model_base.h"

#include "ma_model_classifier.h"
#include "ma_model_detector.h"
#include "ma_model_point_detector.h"
#include "ma_model_pose_detector.h"

#include "ma_model_fomo.h"
#include "ma_model_nvidia_det.h"
#include "ma_model_pfld.h"
#include "ma_model_yolo11.h"
#include "ma_model_yolo11_pose.h"
#include "ma_model_yolo11_seg.h"
#include "ma_model_yolo_world.h"
#include "ma_model_yolov5.h"
#include "ma_model_yolov8.h"
#include "ma_model_yolov8_pose.h"
#include "ma_model_yolov8_pose_hailo.h"
#include "ma_model_yolov8_seg_hailo.h"
#include "ma_model_rtmdet.h"

namespace ma {

using namespace ma::engine;

class ModelFactory {
public:
    static Model* create(Engine* engine, size_t algorithm_id = 0);
    static ma_err_t remove(Model* model);
};

}  // namespace ma


#endif  // _MA_MODEL_FACTORY_H_