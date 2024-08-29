#include "stream.h"

namespace ma::server::node {

static constexpr char TAG[] = "ma::server::node::stream";

StreamNode::StreamNode(std::string id)
    : Node("stream", id), url_(""), thread_(nullptr), camera_(nullptr), frame_(1) {}

StreamNode::~StreamNode() {
    if (thread_ != nullptr) {
        delete thread_;
    }
}

void StreamNode::threadEntry() {

    // waiting for camera create
    while (camera_ == nullptr) {
        if (!NodeFactory::getNodes("camera").empty()) {
            camera_ = static_cast<CameraNode*>(NodeFactory::getNodes("camera")[0]);
            break;
        }
        Thread::sleep(Tick::fromMilliseconds(100));
    }

    camera_->attach(CHN_H264, &frame_);

    camera_->start();

    while (true) {
        videoFrame* frame = nullptr;
        if (frame_.fetch(reinterpret_cast<void**>(&frame))) {
            MA_LOGD(TAG, "release frame: %p %d", frame, frame->ref_cnt.load(std::memory_order_seq_cst));
            Thread::sleep(Tick::fromMilliseconds(5));
            frame->release();
        }
    }
}

void StreamNode::threadEntryStub(void* obj) {
    reinterpret_cast<StreamNode*>(obj)->threadEntry();
}

ma_err_t StreamNode::onCreate(const json& config, const Response& response) {
    Guard guard(mutex_);
    ma_err_t err = MA_OK;
    response(json::object(
        {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", err}, {"data", ""}}));
    thread_ = new Thread((type_ + "#" + id_).c_str(), threadEntryStub);
    thread_->start(this);
    return err;
}

ma_err_t StreamNode::onMessage(const json& message, const Response& response) {
    Guard guard(mutex_);
    ma_err_t err = MA_OK;
    response(json::object(
        {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", err}, {"data", ""}}));
    return err;
}

ma_err_t StreamNode::onDestroy(const Response& response) {
    Guard guard(mutex_);
    ma_err_t err = MA_OK;
    if (thread_ != nullptr) {
        delete thread_;
        thread_ = nullptr;
    }
    response(json::object(
        {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", err}, {"data", ""}}));

    return MA_OK;
}

REGISTER_NODE("stream", StreamNode);

}  // namespace ma::server::node