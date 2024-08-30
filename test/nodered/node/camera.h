#pragma once

#include "node.h"
#include "server.h"
namespace ma::node {

enum { CHN_RAW = 0, CHN_JPEG = 1, CHN_H264 = 2, CHN_MAX };

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    ma_pixel_format_t format;
    bool configured;
    bool enabled;
    std::vector<MessageBox*> msgboxes;
} channel;

class videoFrame {
public:
    videoFrame() : ref_cnt(0) {
        memset(&img, 0, sizeof(ma_img_t));
    }
    ~videoFrame() = default;
    inline void ref(int n = 1) {
        ref_cnt.fetch_add(n, std::memory_order_relaxed);
    }
    inline void release() {
        if (ref_cnt.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }
    ma_tick_t timestamp;
    std::atomic<int> ref_cnt;
    ma_img_t img;
};

class CameraNode : public Node {

public:
    CameraNode(std::string id);
    ~CameraNode();

    ma_err_t onCreate(const json& config) override;
    ma_err_t onStart() override;
    ma_err_t onMessage(const json& message) override;
    ma_err_t onStop() override;
    ma_err_t onDestroy() override;

protected:
    void threadEntry();
    static void threadEntryStub(void* obj);

protected:
    std::vector<channel> channels_;
    std::atomic<bool> started_;
    Thread* thread_;
};

}  // namespace ma::node