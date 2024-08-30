#include "camera.h"

namespace ma::node {

static constexpr char TAG[] = "ma::node::camera";

static uint8_t test_buf[32 * 32 * 3] = {0};

CameraNode::CameraNode(std::string id)
    : Node("camera", std::move(id)), started_(false), channels_(CHN_MAX), thread_(nullptr) {
    for (int i = 0; i < CHN_MAX; i++) {
        channels_[i].configured = false;
        channels_[i].enabled    = false;
    }
}

CameraNode::~CameraNode() = default;

void CameraNode::threadEntry() {
    while (1) {
        Guard guard(mutex_);
        Thread::sleep(Tick::fromMilliseconds(30));
        videoFrame* jpeg = new videoFrame();
        jpeg->timestamp  = Tick::current();
        jpeg->img.format = MA_PIXEL_FORMAT_JPEG;
        jpeg->img.width  = 640;
        jpeg->img.height = 640;
        jpeg->ref(channels_[CHN_JPEG].msgboxes.size());

        for (auto msgbox : channels_[CHN_JPEG].msgboxes) {
            jpeg->ref();
            if (msgbox->post(jpeg, Tick::fromMilliseconds(30))) {
                jpeg->release();
            }
        }

        videoFrame* raw = new videoFrame();
        raw->timestamp  = Tick::current();
        raw->img.format = MA_PIXEL_FORMAT_RGB888;
        raw->img.width  = 640;
        raw->img.height = 640;
        raw->ref(channels_[CHN_RAW].msgboxes.size());

        for (auto msgbox : channels_[CHN_RAW].msgboxes) {
            if (!msgbox->post(raw, Tick::fromMilliseconds(30))) {
                raw->release();
            }
        }

        videoFrame* h264 = new videoFrame();
        h264->timestamp  = Tick::current();
        h264->img.format = MA_PIXEL_FORMAT_H264;
        h264->img.width  = 1920;
        h264->img.height = 1080;
        h264->img.data   = test_buf;
        h264->img.size   = sizeof(test_buf);
        h264->ref(channels_[CHN_H264].msgboxes.size());
        for (auto msgbox : channels_[CHN_H264].msgboxes) {
            if (!msgbox->post(h264, Tick::fromMilliseconds(30))) {
                h264->release();
            }
        }
    }
}

void CameraNode::threadEntryStub(void* obj) {
    reinterpret_cast<CameraNode*>(obj)->threadEntry();
}

ma_err_t CameraNode::onCreate(const json& config) {
    Guard guard(mutex_);
    MA_LOGD(TAG, "onCreate: %s", config.dump().c_str());

    if (config.is_array()) {
        for (auto& item : config) {
            int channel = item["channel"].get<int>();
            if (channel < 0 || channel >= CHN_MAX) {
                throw NodeException(MA_EINVAL, "invalid channel");
            }
            channels_[channel].width       = item["width"];
            channels_[channel].height      = item["height"];
            channels_[channel].fps         = item["fps"];
            channels_[channel].format      = item["format"];
            channels_[channel].enabled     = item["enabled"];
            channels_[CHN_H264].configured = item["configured"];
        }
    }

    thread_ = new Thread((type_ + "#" + id_).c_str(), threadEntryStub);

    if (thread_ == nullptr) {
        throw NodeException(MA_ENOMEM, "thread create failed");
    }
    server_->response(
        id_,
        json::object(
            {{"type", MA_MSG_TYPE_RESP}, {"name", "create"}, {"code", MA_OK}, {"data", ""}}));

    return MA_OK;
}

ma_err_t CameraNode::onMessage(const json& message) {
    Guard guard(mutex_);
    MA_LOGD(TAG, "onMessage: %s", message.dump().c_str());
    server_->response(id_,
                      json::object({{"type", MA_MSG_TYPE_RESP},
                                    {"name", message["name"]},
                                    {"code", MA_ENOTSUP},
                                    {"data", ""}}));
    return MA_OK;
}

ma_err_t CameraNode::onDestroy() {
    Guard guard(mutex_);
    MA_LOGD(TAG, "onDestroy");
    if (thread_ != nullptr) {
        delete thread_;
        thread_ = nullptr;
    }

    server_->response(
        id_,
        json::object(
            {{"type", MA_MSG_TYPE_RESP}, {"name", "destroy"}, {"code", MA_OK}, {"data", ""}}));

    return MA_OK;
}

ma_err_t CameraNode::start() {
    if (started_) {
        return MA_OK;
    }
    for (int i = 0; i < CHN_MAX; i++) {
        if (channels_[i].enabled && !channels_[i].configured) {
            MA_LOGW(TAG, "chn %d not configured", i);
            return MA_AGAIN;
        }
    }
    MA_LOGD(TAG, "started");
    started_ = true;
    thread_->start(this);
    return MA_OK;
}

ma_err_t CameraNode::config(int chn,
                            uint32_t width,
                            uint32_t height,
                            uint32_t fps,
                            ma_pixel_format_t format,
                            bool enabled) {
    Guard guard(mutex_);
    if (chn < 0 || chn >= CHN_MAX) {
        return MA_EINVAL;
    }
    if (channels_[chn].configured) {
        return MA_EBUSY;
    }
    channels_[chn].width      = width;
    channels_[chn].height     = height;
    channels_[chn].fps        = fps;
    channels_[chn].format     = format;
    channels_[chn].enabled    = enabled;
    channels_[chn].configured = true;
    return MA_OK;
}

ma_err_t CameraNode::attach(int chn, MessageBox* msgbox) {
    Guard guard(mutex_);

    if (channels_[chn].enabled) {
        MA_LOGV(TAG, "attach %p to %d", msgbox, chn);
        channels_[chn].msgboxes.push_back(msgbox);
    }

    return MA_OK;
}

ma_err_t CameraNode::detach(int chn, MessageBox* msgbox) {
    Guard guard(mutex_);

    auto it = std::find(channels_[chn].msgboxes.begin(), channels_[chn].msgboxes.end(), msgbox);
    if (it != channels_[chn].msgboxes.end()) {
        channels_[chn].msgboxes.erase(it);
    }
    return MA_OK;
}

REGISTER_NODE_SINGLETON("camera", CameraNode);


}  // namespace ma::node