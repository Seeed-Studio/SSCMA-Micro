#ifndef _MA_CAMERA_H_
#define _MA_CAMERA_H_

#include <core/ma_config_internal.h>
#include <core/ma_types.h>

#include <string>

#include "ma_sensor.h"

namespace ma {

enum ma_camera_stream_mode_e {
    MA_CAMERA_STREAM_MODE_UNKNOWN = 0,
    MA_CAMERA_STREAM_MODE_REFRESH_ON_RETURN,
    MA_CAMERA_STREAM_MODE_REFRESH_ON_RETREIVE,
};

enum ma_camera_ctrl_e {
    MA_CAMERA_CTRL_UNKNOWN = 0,
    MA_CAMERA_CTRL_EXPOSURE,
    MA_CAMERA_CTRL_GAIN,
    MA_CAMERA_CTRL_WHITE_BALANCE,
    MA_CAMERA_CTRL_FOCUS,
    MA_CAMERA_CTRL_ZOOM,
    MA_CAMERA_CTRL_PAN,
    MA_CAMERA_CTRL_TILT,
    MA_CAMERA_CTRL_IRIS,
    MA_CAMERA_CTRL_SHUTTER,
    MA_CAMERA_CTRL_BRIGHTNESS,
    MA_CAMERA_CTRL_CONTRAST,
    MA_CAMERA_CTRL_SATURATION,
    MA_CAMERA_CTRL_HUE,
    MA_CAMERA_CTRL_SHARPNESS,
    MA_CAMERA_CTRL_GAMMA,
    MA_CAMERA_CTRL_COLOR_TEMPERATURE,
    MA_CAMERA_CTRL_BACKLIGHT_COMPENSATION,
    MA_CAMERA_CTRL_ROTATE,
    MA_CAMERA_CTRL_REGISTER
};

enum ma_camera_ctrl_model_e {
    MA_CAMERA_CTRL_MODEL_UNKNOWN = 0,
    MA_CAMERA_CTRL_MODEL_READ,
    MA_CAMERA_CTRL_MODEL_WRITE,
};

union ma_camera_ctrl_value_t {
    uint8_t  bytes[4];
    uint16_t u16[2];
    int32_t  i32;
    float    f32;
};

class Camera : public Sensor {
   public:
    Camera() noexcept
        : Sensor(MA_SENSOR_TYPE_CAMERA), m_streaming(false), m_stream_mode(MA_CAMERA_STREAM_MODE_UNKNOWN) {}
    virtual ~Camera() = default;

    [[nodiscard]] virtual ma_err_t startStream(ma_camera_stream_mode_e mode) noexcept = 0;
    virtual void                   stopStream() noexcept                              = 0;

    [[nodiscard]] virtual ma_err_t commandCtrl(ma_camera_ctrl_e        ctrl,
                                               ma_camera_ctrl_model_e  mode,
                                               ma_camera_ctrl_value_t& value) noexcept = 0;

    [[nodiscard]] virtual ma_err_t retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) noexcept = 0;
    virtual void                   returnFrame(ma_img_t& frame) noexcept                             = 0;

    [[nodiscard]] bool isStreaming() const noexcept { return m_streaming; }

   private:
    Camera(const Camera&)            = delete;
    Camera& operator=(const Camera&) = delete;

   protected:
    bool                    m_streaming;
    ma_camera_stream_mode_e m_stream_mode;
};

}  // namespace ma

#endif