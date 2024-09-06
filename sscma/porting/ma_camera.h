#ifndef _MA_CAMERA_H_
#define _MA_CAMERA_H_

#include <core/ma_config_internal.h>
#include <core/ma_types.h>

#include "ma_sensor.h"

namespace ma {

enum ma_camera_stream_mode_e {
    MA_CAMERA_STREAM_MODE_UNKNOWN             = 0,
    MA_CAMERA_STREAM_MODE_REFRESH_ON_RETURN   = 1,
    MA_CAMERA_STREAM_MODE_REFRESH_ON_RETREIVE = 2,
};

class Camera : public Sensor {
   public:
    Camera() : Sensor(MA_SENSOR_TYPE_CAMERA), m_streaming(false), m_stream_mode(MA_CAMERA_STREAM_MODE_UNKNOWN) {}
    virtual ~Camera() = default;

    virtual ma_err_t startStream(ma_camera_stream_mode_e mode) = 0;
    virtual ma_err_t stopStream()                              = 0;

    virtual ma_err_t retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) = 0;
    virtual void     returnFrame(ma_img_t& frame)                             = 0;

   private:
    bool                    m_streaming;
    ma_camera_stream_mode_e m_stream_mode;
};

}  // namespace ma

#endif