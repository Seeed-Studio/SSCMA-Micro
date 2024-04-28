

#include "drv_common.h"

#include <core/el_debug.h>
#include <core/el_types.h>
#include <inttypes.h>
#include <string.h>

#include "drv_shared_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <WE2_core.h>
#include <hx_drv_gpio.h>
#include <hx_drv_inp.h>
#include <hx_drv_jpeg.h>
#include <hx_drv_scu.h>
#include <hx_drv_scu_export.h>
#include <hx_drv_timer.h>
#include <hx_drv_xdma.h>
#include <sensor_dp_lib.h>

#ifdef __cplusplus
}
#endif

volatile bool     _initiated_before  = false;
volatile bool     _frame_ready       = false;
volatile uint32_t _frame_count       = 0;
volatile uint32_t _wdma1_baseaddr    = WDMA_1_BASE_ADDR;
volatile uint32_t _wdma2_baseaddr    = JPEG_BASE_ADDR;
volatile uint32_t _wdma3_baseaddr    = YUV422_BASE_ADDR;
volatile uint32_t _jpegsize_baseaddr = JPEG_FILL_BASE_ADDR;
el_img_t          _frame;
el_img_t          _jpeg;

void _reset_all_wdma_buffer() {
    memset((void*)_wdma1_baseaddr, 0, WDMA_1_BASE_SIZE);
    memset((void*)_wdma2_baseaddr, 0, JPEG_BASE_SIZE);
    memset((void*)_wdma3_baseaddr, 0, YUV422_BASE_SIZE);
    memset((void*)_jpegsize_baseaddr, 0, JPEG_FILL_SIZE);
}

void (*_drv_dp_event_cb_on_frame_ready)() = NULL;
void (*_drv_dp_on_stop_stream)() = NULL;

void _drv_dp_event_cb(SENSORDPLIB_STATUS_E event) {
    EL_LOGD("event: %d", event);

    switch (event) {
    case SENSORDPLIB_STATUS_XDMA_FRAME_READY:
        _frame_ready = true;
        ++_frame_count;
        if (_drv_dp_event_cb_on_frame_ready != NULL) {
            _drv_dp_event_cb_on_frame_ready();
        }
        break;
    default:
        _initiated_before = false;
        EL_LOGW("unkonw event: %d", event);
        break;
    }
}

el_err_code_t _drv_capture(uint32_t timeout) {
    uint32_t time = el_get_time_ms();

    while (!_frame_ready) {
        if (el_get_time_ms() - time >= timeout) {
            EL_LOGD("frame timeout\n");
            _initiated_before = false;
            return EL_ETIMOUT;
        }
        el_sleep(3);
    }

    return EL_OK;
}

el_err_code_t _drv_capture_stop() {
    _frame_ready = false;

    if (_drv_dp_on_stop_stream != NULL) {
        _drv_dp_on_stop_stream();
    }

    sensordplib_retrigger_capture();

    return EL_OK;
}

el_img_t _drv_get_frame() { return _frame; }

el_img_t _drv_get_jpeg() {
    uint8_t  frame_no  = 0;
    uint8_t  buffer_no = 0;
    uint32_t reg_val   = 0;
    uint32_t mem_val   = 0;

    hx_drv_xdma_get_WDMA2_bufferNo(&buffer_no);
    hx_drv_xdma_get_WDMA2NextFrameIdx(&frame_no);

    if (frame_no == 0) {
        frame_no = buffer_no - 1;
    } else {
        frame_no = frame_no - 1;
    }

    hx_drv_jpeg_get_EncOutRealMEMSize(&reg_val);
    hx_drv_jpeg_get_FillFileSizeToMem(frame_no, (uint32_t)_jpegsize_baseaddr, &mem_val);
    hx_drv_jpeg_get_MemAddrByFrameNo(frame_no, _wdma2_baseaddr, &_jpeg.data);

    _jpeg.size = mem_val == reg_val ? mem_val : reg_val;

    hx_InvalidateDCache_by_Addr((volatile void*)_jpeg.data, _jpeg.size);

    return _jpeg;
}
