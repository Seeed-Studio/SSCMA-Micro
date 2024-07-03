#ifndef MA_CAMERA_H_
#define MA_CAMERA_H_

#include <atomic>

#include "core/ma_common.h"
#include "porting/ma_osal.h"

namespace ma {

class Camera {
public:
    Camera()          = default;
    virtual ~Camera() = default;

    virtual ma_err_t open()       = 0;
    virtual ma_err_t close()      = 0;
    virtual operator bool() const = 0;

    bool isOpened() const {
        return m_opened.load();
    }

    bool isStreaming() const {
        return m_streaming.load();
    }

    virtual ma_err_t start()                                = 0;  // start streaming
    virtual ma_err_t stop()                                 = 0;  // stop streaming
    virtual ma_err_t capture()                              = 0;  // capture a frame
    virtual ma_err_t read(ma_img_t* img, ma_tick_t timeout) = 0;  // read a frame if available
    virtual ma_err_t release(ma_img_t* img)                 = 0;  // release a frame

protected:
    std::atomic<bool> m_opened;
    std::atomic<bool> m_streaming;
};

class CameraProvider {
public:
    CameraProvider()          = default;
    virtual ~CameraProvider() = default;

    virtual operator bool() const = 0;

    bool isOpened() const {
        return m_opened.load();
    }

    bool isStreaming() const {
        return m_streaming.load();
    }

    void setCallback(std::function<ma_err_t(ma_camera_event_t)> callback) {
        m_callback = std::move(callback);
    }

    virtual ma_err_t write(const ma_img_t* img) = 0;

protected:
    std::atomic<bool> m_opened;
    std::atomic<bool> m_streaming;
    std::function<ma_err_t(ma_camera_event_t)> m_callback;
};

}  // namespace ma

#endif  // MA_CAMERA_H_