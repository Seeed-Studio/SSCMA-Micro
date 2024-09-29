#include "ma_camera_himax.h"

#include <core/ma_debug.h>

#include "drivers/camera/drv_common.h"
#include "drivers/camera/drv_imx219.h"
#include "drivers/camera/drv_imx708.h"
#include "drivers/camera/drv_ov5647.h"

namespace ma {

static ma_err_t (*_drv_cam_init)(uint16_t, uint16_t, int) = nullptr;
static void (*_drv_cam_deinit)()                          = nullptr;

struct presets_wrapper_t {
    const char* description;
    uint16_t    width;
    uint16_t    height;
    int         compression;
};

static const presets_wrapper_t _presets[] = {
  { "240x240 Small", 240, 240, 10},
  { "416x416 Small", 416, 416, 10},
  { "480x480 Small", 480, 480, 10},
  { "640x480 Small", 640, 480, 10},
  {"240x240 Medium", 240, 240,  4},
  {"416x416 Medium", 416, 416,  4},
  {"480x480 Medium", 480, 480,  4},
};

static ma_pixel_rotate_t _rotation_override      = MA_PIXEL_ROTATE_0;
static volatile int      _frame_shared_ref_count = 0;
// ^ shared pointer or unique pointer would be better but currently is not allowed

CameraHimax::CameraHimax(size_t id) : Camera(id) {
    for (const auto& preset : _presets) {
        m_presets.push_back({.description = preset.description});
    }
    m_presets.shrink_to_fit();
}

CameraHimax::~CameraHimax() { deInit(); }

ma_err_t CameraHimax::init(size_t preset_idx) noexcept {
    if (m_initialized) [[unlikely]] {
        return MA_OK;
    }

    preset_idx = preset_idx ? preset_idx : 3;
    if (preset_idx >= m_presets.size()) [[unlikely]] {
        return MA_EINVAL;
    }
    m_preset_idx = preset_idx;

    if (drv_ov5647_probe() == MA_OK) {
        _drv_cam_init   = drv_ov5647_init;
        _drv_cam_deinit = drv_ov5647_deinit;
    } else if (drv_imx708_probe() == MA_OK) {
        _drv_cam_init   = drv_imx708_init;
        _drv_cam_deinit = drv_imx708_deinit;
    } else {
        return MA_ENOTSUP;
    }

    const auto& preset = _presets[m_preset_idx - 1];
    auto        ec     = _drv_cam_init(preset.width, preset.height, preset.compression);
    if (ec != MA_OK) {
        return ec;
    }

    m_initialized = true;

    return MA_OK;
}

void CameraHimax::deInit() noexcept {
    if (!m_initialized) [[unlikely]] {
        return;
    }

    if (_drv_cam_deinit) [[likely]] {
        _drv_cam_deinit();
    }

    m_initialized = false;
}

ma_err_t CameraHimax::startStream(StreamMode mode) noexcept {
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

void CameraHimax::stopStream() noexcept {
    if (!m_initialized) [[unlikely]] {
        return;
    }

    if (!m_streaming) [[unlikely]] {
        return;
    }

    if (_frame_shared_ref_count > 0) {
        MA_LOGE(MA_TAG, "Streaming stopped with %d frames unreturned", _frame_shared_ref_count);
        _frame_shared_ref_count = 0;
    }

    m_streaming = false;
}

ma_err_t CameraHimax::commandCtrl(CtrlType ctrl, CtrlMode mode, CtrlValue& value) noexcept {
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
    case kRegister: {
        switch (mode) {
        case kWrite:
            return drv_set_reg(value.u16s[0], value.bytes[2]);
        case kRead:
            return drv_get_reg(value.u16s[0], &value.bytes[2]);
        default:
            return MA_EINVAL;
        }
    } break;
    default:
        return MA_ENOTSUP;
    }

    return MA_OK;
}

ma_err_t CameraHimax::retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) noexcept {
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
            drv_capture_next();
            break;
        default:
            break;
        }
        auto ret = drv_capture(1000);
        if (ret != MA_OK) {
            return ret;
        }
    }

    switch (format) {
    case MA_PIXEL_FORMAT_AUTO:
    case MA_PIXEL_FORMAT_YUV422:
        frame = drv_get_frame();
        break;
    case MA_PIXEL_FORMAT_JPEG:
        frame = drv_get_jpeg();
        break;
    default:
        return MA_ENOTSUP;
    }

    frame.rotate = _rotation_override;
    ++_frame_shared_ref_count;

    return MA_OK;
}

void CameraHimax::returnFrame(ma_img_t& frame) noexcept {
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
    // ^ ref count is not thread safe and we don't have a simple way to check if the frame is really borrowed from here

    if (_frame_shared_ref_count == 0) {
        switch (m_stream_mode) {
        case kRefreshOnReturn:
            drv_capture_next();
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