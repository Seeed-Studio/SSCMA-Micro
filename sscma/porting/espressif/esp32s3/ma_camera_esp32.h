#ifndef _MA_CAMERA_HIMAX_H_
#define _MA_CAMERA_HIMAX_H_

#include <porting/ma_camera.h>

#include "ma_config_board.h"

namespace ma {

class CameraESP32 final : public Camera {
   public:
    CameraESP32(size_t id);
    ~CameraESP32();

    ma_err_t init(size_t preset_idx) noexcept override;
    void     deInit() noexcept override;

    ma_err_t startStream(StreamMode mode) noexcept override;
    void     stopStream() noexcept override;

    ma_err_t commandCtrl(CtrlType ctrl, CtrlMode mode, CtrlValue& value) noexcept override;

    ma_err_t retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) noexcept override;
    void     returnFrame(ma_img_t& frame) noexcept override;
};

}  // namespace ma

#endif