#ifndef MA_CAMERA_SOPHGO_H_
#define MA_CAMERA_SOPHGO_H_

#include <forward_list>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

#include "core/ma_common.h"
#include "porting/ma_camera.h"
#include "porting/ma_osal.h"

#include "app_ipcam_paramparse.h"
#include "video.h"

namespace ma {
using isp_callback_t = int (*)(void* pdata, void* pargs);


class CameraSophgo : public Camera {
public:
    CameraSophgo(uint8_t index = 0);
    ~CameraSophgo();

    enum { ISP_CH0 = 0, ISP_CH1 = 1, ISP_CH2 = 2, ISP_CH_MAX };

    operator bool() const override;

    ma_err_t open(uint16_t width,
                  uint16_t height,
                  ma_pixel_format_t format = MA_PIXEL_FORMAT_RGB888,
                  int fps                  = 2) override;
    ma_err_t close() override;
    ma_err_t start() override;
    ma_err_t stop() override;
    ma_err_t capture() override;
    ma_err_t read(ma_img_t* img, ma_tick_t timeout = Tick::waitForever) override;
    ma_err_t release(ma_img_t* img) override;

private:
    Mutex m_mutex;
    MessageBox m_msgBox;

protected:
    void callback(void* pdata);

private:
    static Mutex s_mutex;
    static std::atomic<bool> s_opened;
    static std::atomic<bool> s_streaming;
    static Camera* s_attach[ISP_CH_MAX];
    static int callbackStub(void* pdata, void* pargs);
};
}  // namespace ma
#endif
