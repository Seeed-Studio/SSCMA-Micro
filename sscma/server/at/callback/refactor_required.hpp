#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "core/ma_core.h"
#include "core/utils/ma_base64.h"
#include "ma_codec.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

namespace ma {

using namespace ma::model;

ma_err_t setAlgorithmInput(Model* algorithm, ma_img_t& img) {
    if (algorithm == nullptr) {
        return MA_EINVAL;
    }

    switch (algorithm->getType()) {
    case MA_MODEL_TYPE_PFLD:
        return static_cast<PointDetector*>(algorithm)->run(&img);

    case MA_MODEL_TYPE_IMCLS:
        return static_cast<Classifier*>(algorithm)->run(&img);

    case MA_MODEL_TYPE_FOMO:
    case MA_MODEL_TYPE_YOLOV5:
    case MA_MODEL_TYPE_YOLOV8:
    case MA_MODEL_TYPE_NVIDIA_DET:
    case MA_MODEL_TYPE_YOLO_WORLD:
        return static_cast<Detector*>(algorithm)->run(&img);

    case MA_MODEL_TYPE_YOLOV8_POSE:
        return static_cast<PoseDetector*>(algorithm)->run(&img);

    default:
        return MA_ENOTSUP;
    }
}

ma_err_t serializeAlgorithmOutput(Model* algorithm, Encoder* encoder) {
    if (algorithm == nullptr || encoder == nullptr) {
        return MA_EINVAL;
    }

    auto ret = MA_OK;

    switch (algorithm->getType()) {
    case MA_MODEL_TYPE_PFLD: {
        const auto& results = static_cast<PointDetector*>(algorithm)->getResults();
        // encoder->write(results);
        break;
    }

    case MA_MODEL_TYPE_IMCLS: {
        const auto& results = static_cast<Classifier*>(algorithm)->getResults();
        ret                 = encoder->write(results);
        break;
    }

    case MA_MODEL_TYPE_FOMO:
    case MA_MODEL_TYPE_YOLOV5:
    case MA_MODEL_TYPE_YOLOV8:
    case MA_MODEL_TYPE_NVIDIA_DET:
    case MA_MODEL_TYPE_YOLO_WORLD: {
        const auto& results = static_cast<Detector*>(algorithm)->getResults();
        ret                 = encoder->write(results);
        break;
    }

    case MA_MODEL_TYPE_YOLOV8_POSE: {
        const auto& results = static_cast<PoseDetector*>(algorithm)->getResults();
        ret                 = encoder->write(results);
        break;
    }

    default:
        ret = MA_ENOTSUP;
    }

    if (ret != MA_OK) {
        MA_LOGD(MA_TAG, "Failed to serialize algorithm output: %d", ret);
    }

    return ret;
}

}  // namespace ma
