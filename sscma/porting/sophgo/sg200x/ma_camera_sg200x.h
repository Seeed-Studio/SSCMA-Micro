#ifndef _MA_CAMERA_SG200X_H_
#define _MA_CAMERA_SG200X_H_

#include <map>

#include <core/ma_common.h>
#include <porting/ma_porting.h>

#include "video.h"

#include "ma_config_board.h"

namespace ma {

class CameraSG200X final : public Camera {

    enum { CHN_RAW = 0, CHN_JPEG = 1, CHN_H264 = 2, CHN_MAX };

    typedef struct {
        int16_t width;
        int16_t height;
        int16_t fps;
        ma_pixel_format_t format;
        bool configured;
        bool enabled;
        MessageBox* queue;
        Semaphore sem;
    } channel;

public:
    CameraSG200X(size_t id);
    ~CameraSG200X();

    ma_err_t init(size_t preset_idx) noexcept override;
    void deInit() noexcept override;

    ma_err_t startStream(StreamMode mode) noexcept override;
    void stopStream() noexcept override;

    ma_err_t commandCtrl(CtrlType ctrl, CtrlMode mode, CtrlValue& value) noexcept override;

    ma_err_t retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) noexcept override;
    void returnFrame(ma_img_t& frame) noexcept override;

private:
    channel m_channels[CHN_MAX];

    int chn;
    int vencCallback(void* pData, void* pArgs);
    int vpssCallback(void* pData, void* pArgs);
    static int vencCallbackStub(void* pData, void* pArgs, void* pUserData);
    static int vpssCallbackStub(void* pData, void* pArgs, void* pUserData);
};

}  // namespace ma

#endif