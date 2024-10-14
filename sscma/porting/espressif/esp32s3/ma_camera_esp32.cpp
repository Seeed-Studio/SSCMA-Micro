#include "ma_camera_esp32.h"
#include <esp_camera.h>
#include <esp_log.h>

#include "core/cv/ma_cv.h"
#include "core/ma_debug.h"
#include <vector>

#define XCLK_FREQ_HZ 15000000

struct presets_wrapper_t {
    const char* description;
    uint16_t width;
    uint16_t height;
};

static const presets_wrapper_t _presets[] = {
    {"240x240 Small", 240, 240},
};

static camera_config_t _config;
static camera_fb_t* _fb  = nullptr;
static sensor_t* _sensor = nullptr;
static std::vector<uint8_t> _jpeg_buffer;

static ma_pixel_rotate_t _rotation_override = MA_PIXEL_ROTATE_0;
static volatile int _frame_shared_ref_count = 0;

namespace ma {

CameraESP32::CameraESP32(size_t id) : Camera(id) {
    for (const auto& preset : _presets) {
        m_presets.push_back({.description = preset.description});
    }
    m_presets.shrink_to_fit();
}

CameraESP32::~CameraESP32() {
    deInit();
}

ma_err_t CameraESP32::init(size_t preset_idx) noexcept {
    if (m_initialized) [[unlikely]] {
        return MA_OK;
    }

    preset_idx = preset_idx ? preset_idx : 3;
    if (preset_idx >= m_presets.size()) [[unlikely]] {
        return MA_EINVAL;
    }
    m_preset_idx = preset_idx;


    _config.ledc_channel = LEDC_CHANNEL_0;
    _config.ledc_timer   = LEDC_TIMER_0;
    _config.pin_d0       = CAMERA_PIN_D0;
    _config.pin_d1       = CAMERA_PIN_D1;
    _config.pin_d2       = CAMERA_PIN_D2;
    _config.pin_d3       = CAMERA_PIN_D3;
    _config.pin_d4       = CAMERA_PIN_D4;
    _config.pin_d5       = CAMERA_PIN_D5;
    _config.pin_d6       = CAMERA_PIN_D6;
    _config.pin_d7       = CAMERA_PIN_D7;
    _config.pin_xclk     = CAMERA_PIN_XCLK;
    _config.pin_pclk     = CAMERA_PIN_PCLK;
    _config.pin_vsync    = CAMERA_PIN_VSYNC;
    _config.pin_href     = CAMERA_PIN_HREF;
    _config.pin_sscb_sda = CAMERA_PIN_SIOD;
    _config.pin_sscb_scl = CAMERA_PIN_SIOC;
    _config.pin_pwdn     = CAMERA_PIN_PWDN;
    _config.pin_reset    = CAMERA_PIN_RESET;
    _config.xclk_freq_hz = XCLK_FREQ_HZ;
    _config.pixel_format = PIXFORMAT_RGB565;
    _config.frame_size   = FRAMESIZE_240X240;
    _config.jpeg_quality = 12;
    _config.fb_count     = 1;
    _config.fb_location  = CAMERA_FB_IN_PSRAM;
    _config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;


    esp_err_t err = esp_camera_init(&_config);
    if (err != ESP_OK) {
        MA_LOGE(MA_TAG, "Camera init failed with error 0x%x", err);
        return MA_EIO;
    }

    _sensor = esp_camera_sensor_get();
    if (!_sensor) {
        MA_LOGE(MA_TAG, "Camera sensor not found");
        return MA_EIO;
    }

    _sensor->set_vflip(_sensor, 1);
    _sensor->set_hmirror(_sensor, 1);

    if (_sensor->id.PID == OV3660_PID) {
        _sensor->set_brightness(_sensor, 1);   // up the blightness just a bit
        _sensor->set_saturation(_sensor, -2);  // lower the saturation
    }

    _fb = esp_camera_fb_get();

    m_initialized = true;

    return MA_OK;
}


void CameraESP32::deInit() noexcept {
    if (!m_initialized) [[unlikely]] {
        return;
    }

    esp_camera_deinit();

    _jpeg_buffer.clear();
    _jpeg_buffer.shrink_to_fit();

    m_initialized = false;
}

ma_err_t CameraESP32::startStream(StreamMode mode) noexcept {
    if (!m_initialized) [[unlikely]] {
        return MA_EPERM;
    }

    if (m_streaming) [[unlikely]] {
        return MA_OK;
    }

    m_stream_mode = mode;

    m_streaming = true;

    return MA_OK;
}

void CameraESP32::stopStream() noexcept {
    if (!m_initialized) [[unlikely]] {
        return;
    }

    if (!m_streaming) [[unlikely]] {
        return;
    }

    if (_fb) {
        esp_camera_fb_return(_fb);
        _fb = nullptr;
    }

    m_streaming = false;
}

ma_err_t CameraESP32::commandCtrl(CtrlType ctrl, CtrlMode mode, CtrlValue& value) noexcept {
    if (!m_initialized) [[unlikely]] {
        return MA_EPERM;
    }

    switch (ctrl) {
        case kRotate: {
            switch (mode) {
                case kWrite:
                    _rotation_override = static_cast<ma_pixel_rotate_t>(value.i32);
                    break;
                case kRead:
                    value.i32 = _rotation_override;
                    break;
                default:
                    return MA_EINVAL;
            }
        } break;

        default:
            return MA_ENOTSUP;
    }


    return MA_OK;
}

ma_err_t CameraESP32::retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) noexcept {
    if (!m_initialized) [[unlikely]] {
        return MA_EPERM;
    }

    if (!m_streaming) [[unlikely]] {
        return MA_EPERM;
    }

    if (frame.data || frame.size) {
        MA_LOGE(MA_TAG, "Frame ownership violation");
        return MA_EINVAL;
    }

    if (_frame_shared_ref_count == 0) {
        switch (m_stream_mode) {
            case kRefreshOnRetrieve:
                if (_fb) {
                    esp_camera_fb_return(_fb);
                    _fb = nullptr;
                }
                _fb = esp_camera_fb_get();
                if (!_fb) {
                    MA_LOGE(MA_TAG, "Camera capture failed");
                    return MA_EIO;
                }
                break;
            default:
                break;
        }
    }

    switch (format) {
        case MA_PIXEL_FORMAT_AUTO:
        case MA_PIXEL_FORMAT_RGB565:
            frame.data   = _fb->buf;
            frame.size   = _fb->len;
            frame.format = MA_PIXEL_FORMAT_RGB565;
            break;
        case MA_PIXEL_FORMAT_JPEG: {
            ma_img_t rgb565_frame;
            rgb565_frame.data   = _fb->buf;
            rgb565_frame.size   = _fb->len;
            rgb565_frame.width  = _fb->width;
            rgb565_frame.height = _fb->height;
            rgb565_frame.format = MA_PIXEL_FORMAT_RGB565;

            size_t estimated_size =  rgb565_frame.width * rgb565_frame.height * 3 / 8;
            if (_jpeg_buffer.size() < estimated_size) {
                _jpeg_buffer.resize(estimated_size);
            }

            frame.data   = _jpeg_buffer.data(); 
            frame.size   = _jpeg_buffer.size();
            if (frame.data == nullptr || frame.size == 0) {
                MA_LOGE(MA_TAG, "JPEG buffer allocation failed");
                return MA_ENOMEM;
            }

            frame.width  = _fb->width;
            frame.height = _fb->height;
            auto ret = ma::cv::rgb_to_jpeg(&rgb565_frame, &frame);
            if (ret != MA_OK) {
                MA_LOGE(MA_TAG, "Failed to convert RGB565 to JPEG");
            }
        }

        break;
        default:
            return MA_ENOTSUP;
    }

    frame.rotate = _rotation_override;
    ++_frame_shared_ref_count;

    return MA_OK;
}

void CameraESP32::returnFrame(ma_img_t& frame) noexcept {
    if (!m_initialized) [[unlikely]] {
        return;
    }

    if (!m_streaming) [[unlikely]] {
        return;
    }

    if (!frame.data) [[unlikely]] {
        MA_LOGW(MA_TAG, "Return empty frame");
        return;
    }

    frame.data = nullptr;
    frame.size = 0;
    --_frame_shared_ref_count;

    if (_frame_shared_ref_count == 0) {
        switch (m_stream_mode) {
            case kRefreshOnReturn:
                if (_fb) {
                    esp_camera_fb_return(_fb);
                    _fb = nullptr;
                }
                _fb = esp_camera_fb_get();
                if (!_fb) {
                    MA_LOGE(MA_TAG, "Camera capture failed");
                    return;
                }
                break;
            default:
                break;
        }
    } else if (_frame_shared_ref_count < 0) {
        MA_LOGE(MA_TAG, "Unowned return frame detected");
        _frame_shared_ref_count = 0;
    }
}


}  // namespace ma