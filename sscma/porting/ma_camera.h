#ifndef MA_CAMERA_H_
#define MA_CAMERA_H_

#include <atomic>
#include <functional>

#include "core/ma_common.h"
#include "porting/ma_osal.h"

namespace ma {

class Camera {
public:
    Camera(uint8_t index = 0)
        : m_opened(false),
          m_streaming(false),
          m_capturing(false),
          m_index(index),
          m_format(MA_PIXEL_FORMAT_RGB888) {};
    virtual ~Camera() = default;

    virtual ma_err_t open(uint16_t width,
                          uint16_t height,
                          ma_pixel_format_t format = MA_PIXEL_FORMAT_RGB888,
                          int fps                  = -1) = 0;
    virtual ma_err_t close()            = 0;
    virtual operator bool() const       = 0;

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
    std::atomic<bool> m_capturing;
    uint16_t m_width;
    uint16_t m_height;
    uint8_t m_index;
    ma_pixel_format_t m_format;
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