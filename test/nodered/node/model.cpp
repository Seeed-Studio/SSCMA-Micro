#include <unistd.h>

#include "model.h"

namespace ma::node {

using namespace ma::engine;
using namespace ma::model;

static constexpr char TAG[] = "ma::node::model";

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

    while (true) {
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

ma_err_t ModelNode::onCreate(const json& config) {
    ma_err_t err = MA_OK;

    Guard guard(mutex_);

    MA_LOGD(TAG, "onCreate: %s", config.dump().c_str());

    if (!config.contains("uri")) {
        throw NodeException(MA_EINVAL, "uri not found");
    }

    uri_ = config["uri"].get<std::string>();


    if (uri_.empty()) {
        throw NodeException(MA_EINVAL, "uri is empty");
    }

    if (access(uri_.c_str(), R_OK) != 0) {
        throw NodeException(MA_EINVAL, "model not found: " + uri_);
    }

    MA_LOGD(TAG, "uri: %s", uri_.c_str());

    try {
        engine_ = new EngineDefault();

        if (engine_ == nullptr) {
            throw NodeException(MA_ENOMEM, "Engine create failed");
        }
        if (engine_->init() != MA_OK) {
            throw NodeException(MA_EINVAL, "Engine init failed");
        }
        if (engine_->load(uri_) != MA_OK) {
            throw NodeException(MA_EINVAL, "Model load failed");
        }
        model_ = ModelFactory::create(engine_);
        if (model_ == nullptr) {
            throw NodeException(MA_EINVAL, "Model create failed");
        }

        {  // extra config
            if (config.contains("tscore")) {
                model_->setConfig(MA_MODEL_CFG_OPT_THRESHOLD, config["tscore"].get<float>());
            }
            if (config.contains("tiou")) {
                model_->setConfig(MA_MODEL_CFG_OPT_NMS, config["tiou"].get<float>());
            }
            if (config.contains("topk")) {
                model_->setConfig(MA_MODEL_CFG_OPT_TOPK, config["tiou"].get<float>());
            }
            if (config.contains("debug")) {
                debug_ = config["debug"].get<bool>();
            }
            if (config.contains("trace")) {
                trace_ = config["trace"].get<bool>();
            }
        }

        thread_ = new Thread(id_.c_str(), &ModelNode::threadEntryStub);
        if (thread_ == nullptr) {
            throw NodeException(MA_ENOMEM, "thread create failed");
        }
        server_->response(
            id_,
            json::object(
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
        throw NodeException(err, e.what());
    }
}


ma_err_t ModelNode::onMessage(const json& msg) {
    Guard guard(mutex_);
    ma_err_t err    = MA_OK;
    std::string cmd = msg["name"].get<std::string>();
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
    MA_LOGD(TAG, "onMessage: %s", cmd.c_str());
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
        server_->response(id_,
                          json::object({{"type", MA_MSG_TYPE_RESP},
                                        {"name", cmd},
                                        {"code", MA_OK},
                                        {"data", msg["data"]}}));
    } else {
        throw NodeException(MA_EINVAL, "invalid cmd: " + cmd);
    }
    return MA_OK;
}

ma_err_t ModelNode::onStart() {
    Guard guard(mutex_);
    MA_LOGD(TAG, "onStart: %s", id_.c_str());
    if (started_) {
        return MA_OK;
    }
    thread_->start(this);
    started_ = true;
    return MA_OK;
}

ma_err_t ModelNode::onStop() {
    Guard guard(mutex_);
    MA_LOGD(TAG, "onStop: %s", id_.c_str());
    if (!started_) {
        return MA_OK;
    }
    thread_->stop();
    started_ = false;
    return MA_OK;
}

ma_err_t ModelNode::onDestroy() {
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

    server_->response(
        id_,
        json::object(
            {{"type", MA_MSG_TYPE_RESP}, {"name", "destroy"}, {"code", MA_OK}, {"data", ""}}));

    return MA_OK;
}

REGISTER_NODE_SINGLETON("model", ModelNode);

}  // namespace ma::node