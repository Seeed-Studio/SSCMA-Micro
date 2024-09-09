#include "ma_camera_himax.h"

namespace ma {

CameraHimax::CameraHimax() : Camera() {
    m_presets.push_back({.description = "240x240 Small"});
    m_presets.push_back({.description = "416x416 Small"});
    m_presets.push_back({.description = "480x480 Small"});
    m_presets.push_back({.description = "640x480 Small"});
    m_presets.push_back({.description = "240x240 Medium"});
    m_presets.push_back({.description = "416x416 Medium"});
    m_presets.push_back({.description = "480x480 Medium"});

    m_presets.shrink_to_fit();
}

CameraHimax::~CameraHimax() {}

ma_err_t CameraHimax::init(size_t preset_idx) noexcept { return MA_OK; }

void CameraHimax::deInit() noexcept {}

ma_err_t CameraHimax::startStream(ma_camera_stream_mode_e mode) noexcept { return MA_OK; }

void CameraHimax::stopStream() noexcept {}

ma_err_t CameraHimax::commandCtrl(ma_camera_ctrl_e        ctrl,
                                  ma_camera_ctrl_model_e  mode,
                                  ma_camera_ctrl_value_t& value) noexcept {
    return MA_OK;
}

ma_err_t CameraHimax::retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) noexcept { return MA_OK; }

void CameraHimax::returnFrame(ma_img_t& frame) noexcept {}

}  // namespace ma