#ifndef _MA_CAMERA_SG200XH_
#define _MA_CAMERA_SG200XH_

#include <porting/ma_camera.h>

#include "ma_config_board.h"

namespace ma {

class CameraSg200x final : public Camera {
public:
    CameraSg200x(size_t id);
    ~CameraSg200x();

    ma_err_t init(size_t preset_idx) noexcept override;
    void deInit() noexcept override;

    ma_err_t startStream(StreamMode mode) noexcept override;
    void stopStream() noexcept override;

    ma_err_t commandCtrl(CtrlType ctrl, CtrlMode mode, CtrlValue& value) noexcept override;

    ma_err_t retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) noexcept override;
    void returnFrame(ma_img_t& frame) noexcept override;
};

}  // namespace ma

#endif