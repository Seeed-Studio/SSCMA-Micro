#include <unistd.h>

#include "model.h"

namespace ma::server::node {

using namespace ma::engine;
using namespace ma::model;

static constexpr char TAG[] = "ma::server::node::model";

ModelNode::ModelNode(std::string id)
    : Node("model", id),
      uri_(""),
      debug_(true),
      engine_(nullptr),
      model_(nullptr),
      thread_(nullptr),
      camera_(nullptr),
      raw_frame_(1),
      jpeg_frame_(1) {}

ModelNode::~ModelNode() {
    if (engine_ != nullptr) {
        delete engine_;
    }
    if (model_ != nullptr) {
        delete model_;
    }
    if (thread_ != nullptr) {
        delete thread_;
    }
}

void ModelNode::threadEntry() {

    const ma_img_t* img    = nullptr;
    Detector* detector     = nullptr;
    Classifier* classifier = nullptr;


    switch (model_->getType()) {
        case MA_MODEL_TYPE_FOMO:
        case MA_MODEL_TYPE_YOLOV8:
        case MA_MODEL_TYPE_YOLO_WORLD:
        case MA_MODEL_TYPE_NVIDIA_DET:
        case MA_MODEL_TYPE_YOLOV5:
            detector = static_cast<Detector*>(model_);
            img      = detector->getInputImg();
            break;
        case MA_MODEL_TYPE_IMCLS:
            classifier = static_cast<Classifier*>(model_);
            img        = classifier->getInputImg();
            break;
        default:
            break;
    }

    // waiting for camera create
    while (camera_ == nullptr) {
        if (!NodeFactory::getNodes("camera").empty()) {
            camera_ = static_cast<CameraNode*>(NodeFactory::getNodes("camera")[0]);
            break;
        }
        Thread::sleep(Tick::fromMilliseconds(100));
    }

    camera_->config(CHN_RAW, img->width, img->height, 30, img->format);
    camera_->attach(CHN_RAW, &raw_frame_);

    if (debug_) {
        camera_->config(CHN_JPEG, img->width, img->height, 30, MA_PIXEL_FORMAT_JPEG);
        camera_->attach(CHN_JPEG, &jpeg_frame_);
    }

    camera_->start();

    while (true) {
        Guard guard(mutex_);
        videoFrame* frame_ = nullptr;
        if (raw_frame_.fetch(reinterpret_cast<void**>(&frame_))) {
            Thread::sleep(Tick::fromMicroseconds(100));
            frame_->release();
        }
    }
}

void ModelNode::threadEntryStub(void* obj) {
    reinterpret_cast<ModelNode*>(obj)->threadEntry();
}

ma_err_t ModelNode::onCreate(const json& config, const Response& response) {
    ma_err_t err = MA_OK;

    Guard guard(mutex_);

    MA_LOGD(TAG, "onCreate: %s", config.dump().c_str());

    if (!config["data"].contains("config")) {
        response(json::object({{"type", MA_MSG_TYPE_RESP},
                               {"name", "create"},
                               {"code", MA_EINVAL},
                               {"data", "uri missing"}}));
        return MA_EINVAL;
    }

    uri_ = config["data"]["config"]["uri"].get<std::string>();


    if (uri_.empty()) {
        response(json::object(
            {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", MA_EINVAL}, {"data", ""}}));
        return MA_EINVAL;
    }

    if (access(uri_.c_str(), R_OK) != 0) {
        response(json::object(
            {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", MA_ENOENT}, {"data", ""}}));
        return MA_ENOENT;
    }

    MA_LOGD(TAG, "uri: %s", uri_.c_str());

    try {
        engine_ = new EngineDefault();

        if (engine_ == nullptr) {
            err = MA_ENOMEM;
            throw std::runtime_error("Engine create failed");
        }
        if (engine_->init() != MA_OK) {
            err = MA_EINVAL;
            throw std::runtime_error("Engine init failed");
        }
        if (engine_->load(uri_) != MA_OK) {
            err = MA_EINVAL;
            throw std::runtime_error("Engine load failed");
        }
        model_ = ModelFactory::create(engine_);
        if (model_ == nullptr) {
            err = MA_ENOTSUP;
            throw std::runtime_error("Model create failed");
        }

        {  // extra config
            if (config["data"]["config"].contains("tscore")) {
                model_->setConfig(MA_MODEL_CFG_OPT_THRESHOLD,
                                  config["data"]["config"]["tscore"].get<float>());
            }
            if (config["data"]["config"].contains("tiou")) {
                model_->setConfig(MA_MODEL_CFG_OPT_NMS,
                                  config["data"]["config"]["tiou"].get<float>());
            }
            if (config["data"]["config"].contains("topk")) {
                model_->setConfig(MA_MODEL_CFG_OPT_TOPK,
                                  config["data"]["config"]["tiou"].get<float>());
            }
            if (config["data"]["config"].contains("debug")) {
                debug_ = config["data"]["config"]["debug"].get<bool>();
            }
            if (config["data"]["config"].contains("trace")) {
                trace_ = config["data"]["config"]["trace"].get<bool>();
            }
        }

        thread_ = new Thread(id_.c_str(), &ModelNode::threadEntryStub);
        if (thread_ == nullptr) {
            err = MA_ENOMEM;
            throw std::runtime_error("Thread create failed");
        }
        thread_->start(this);
        response(json::object(
            {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", MA_OK}, {"data", ""}}));

        return MA_OK;
    } catch (std::exception& e) {
        if (engine_ != nullptr) {
            delete engine_;
            engine_ = nullptr;
        }
        if (model_ != nullptr) {
            delete model_;
            model_ = nullptr;
        }
        if (thread_ != nullptr) {
            delete thread_;
            thread_ = nullptr;
        }

        MA_LOGW(TAG, "create failed: %s", e.what());
        response(json::object(
            {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", err}, {"data", e.what()}}));
        return MA_EINVAL;
    }
}


ma_err_t ModelNode::onMessage(const json& msg, const Response& response) {
    Guard guard(mutex_);
    ma_err_t err    = MA_OK;
    std::string cmd = msg["name"].get<std::string>();
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    MA_LOGD(TAG, "onMessage: %s", cmd.c_str());
    try {
        if (cmd == "config") {
            if (msg["data"].contains("tscore")) {
                model_->setConfig(MA_MODEL_CFG_OPT_THRESHOLD, msg["data"]["tscore"].get<float>());
            }
            if (msg["data"].contains("tiou")) {
                model_->setConfig(MA_MODEL_CFG_OPT_NMS, msg["data"]["tiou"].get<float>());
            }
            if (msg["data"].contains("topk")) {
                model_->setConfig(MA_MODEL_CFG_OPT_TOPK, msg["data"]["tiou"].get<float>());
            }
            if (msg["data"].contains("debug")) {
                debug_ = msg["data"]["debug"].get<bool>();
            }
            if (msg["data"].contains("trace")) {
                trace_ = msg["data"]["trace"].get<bool>();
            }
            response(json::object({{"type", MA_MSG_TYPE_RESP},
                                   {"name", cmd},
                                   {"code", MA_OK},
                                   {"data", msg["data"]}}));
        } else {
            err = MA_ENOTSUP;
            throw std::runtime_error("Command not supported");
        }

    } catch (std::exception& e) {
        response(json::object(
            {{"type", MA_MSG_TYPE_RESP}, {"name", cmd}, {"code", err}, {"data", e.what()}}));
    }
    return MA_OK;
}

ma_err_t ModelNode::onDestroy(const Response& response) {
    MA_LOGD(TAG, "onDestroy: %s", id_.c_str());
    // stop thread first
    if (thread_ != nullptr) {
        delete thread_;
        thread_ = nullptr;
    }
    Guard guard(mutex_);
    if (engine_ != nullptr) {
        delete engine_;
        engine_ = nullptr;
    }
    if (model_ != nullptr) {
        delete model_;
        model_ = nullptr;
    }
    if (camera_ != nullptr) {
        camera_ = nullptr;
    }

    response(json::object(
        {{"type", MA_MSG_TYPE_RESP}, {"name", "destroy"}, {"code", MA_OK}, {"data", ""}}));
    return MA_OK;
}

REGISTER_NODE_SINGLETON("model", ModelNode);

}  // namespace ma::server::node