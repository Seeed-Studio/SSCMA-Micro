#ifndef _MA_CAMERA_H_
#define _MA_CAMERA_H_

#include <core/ma_config_internal.h>
#include <core/ma_types.h>

#include <string>

#include "ma_sensor.h"

namespace ma {

class Camera : public Sensor {
   public:
    enum StreamMode : int {
        kUnknown,
        kRefreshOnReturn,
        kRefreshOnRetrieve,
    };

    static std::string __repr__(StreamMode mode) noexcept {
        switch (mode) {
        case StreamMode::kRefreshOnReturn:
            return "RefreshOnReturn";
        case StreamMode::kRefreshOnRetrieve:
            return "RefreshOnRetrieve";
        default:
            return "Unknown";
        }
    }

    enum CtrlType : int {
        kWindow,
        kExposure,
        kGain,
        kWhiteBalance,
        kFocus,
        kZoom,
        kPan,
        kTilt,
        kIris,
        kShutter,
        kBrightness,
        kContrast,
        kSaturation,
        kHue,
        kSharpness,
        kGamma,
        kColorTemperature,
        kBacklightCompensation,
        kRotate,
        kFormat,
        kChannel,
        kFps,
        kRegister,
    };

    enum CtrlMode : int {
        kRead,
        kWrite,
    };

    struct CtrlValue {
        union {
            uint8_t  bytes[4];
            uint16_t u16s[2];
            int32_t  i32;
            float    f32;
        };
    };

   public:
    Camera(size_t id) noexcept : Sensor(id, Sensor::Type::kCamera), m_streaming(false), m_stream_mode(kUnknown) {}
    virtual ~Camera() = default;

    [[nodiscard]] virtual ma_err_t startStream(StreamMode mode) noexcept = 0;
    virtual void                   stopStream() noexcept                 = 0;

    [[nodiscard]] virtual ma_err_t commandCtrl(CtrlType ctrl, CtrlMode mode, CtrlValue& value) noexcept = 0;

    [[nodiscard]] virtual ma_err_t retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) noexcept = 0;
    virtual void                   returnFrame(ma_img_t& frame) noexcept                             = 0;

    [[nodiscard]] bool isStreaming() const noexcept { return m_streaming; }

   private:
    Camera(const Camera&)            = delete;
    Camera& operator=(const Camera&) = delete;

   protected:
    bool       m_streaming;
    StreamMode m_stream_mode;
};

}  // namespace ma

#endif