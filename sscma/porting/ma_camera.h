#ifndef MA_CAMERA_H_
#define MA_CAMERA_H_

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
        return m_opened;
    }

    bool isStreaming() const {
        return m_streaming;
    }

    virtual ma_err_t start()                               = 0;  // start streaming
    virtual ma_err_t stop()                                = 0;  // stop streaming
    virtual ma_err_t capture()                             = 0;  // capture a frame
    virtual ma_err_t read(ma_img_t* img, int timeout = -1) = 0;  // read a frame if available

protected:
    bool m_opened;
    bool m_streaming;
};

}  // namespace ma

#endif  // MA_CAMERA_H_