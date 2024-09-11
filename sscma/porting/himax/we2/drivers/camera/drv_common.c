

#include "drv_common.h"

#include <WE2_core.h>
#include <core/ma_debug.h>
#include <core/ma_types.h>
#include <hx_drv_CIS_common.h>
#include <hx_drv_gpio.h>
#include <hx_drv_inp.h>
#include <hx_drv_jpeg.h>
#include <hx_drv_scu.h>
#include <hx_drv_scu_export.h>
#include <hx_drv_timer.h>
#include <hx_drv_xdma.h>
#include <inttypes.h>
#include <porting/ma_misc.h>
#include <sensor_dp_lib.h>
#include <string.h>

#include "drv_shared_cfg.h"
#include "drv_shared_state.h"

volatile bool     _initiated_before  = false;
volatile bool     _frame_ready       = false;
volatile uint32_t _frame_count       = 0;
volatile uint32_t _wdma1_baseaddr    = WDMA_1_BASE_ADDR;
volatile uint32_t _wdma2_baseaddr    = JPEG_BASE_ADDR;
volatile uint32_t _wdma3_baseaddr    = YUV422_BASE_ADDR;
volatile uint32_t _jpegsize_baseaddr = JPEG_FILL_BASE_ADDR;
ma_img_t          _frame;
ma_img_t          _jpeg;

void _reset_all_wdma_buffer() {
    memset((void*)_wdma1_baseaddr, 0, WDMA_1_BASE_SIZE);
    memset((void*)_wdma2_baseaddr, 0, JPEG_BASE_SIZE);
    memset((void*)_wdma3_baseaddr, 0, YUV422_BASE_SIZE);
    memset((void*)_jpegsize_baseaddr, 0, JPEG_FILL_SIZE);

    MA_LOGD(MA_TAG,
            "Memset wdma1[%x], wdma2[%x], wdma3[%x], jpg_sz[%x]",
            _wdma1_baseaddr,
            _wdma2_baseaddr,
            _wdma3_baseaddr,
            _jpegsize_baseaddr);
}

void (*_drv_dp_event_cb_on_frame_ready)() = NULL;
void (*_drv_dp_on_next_stream)()          = NULL;

void _drv_dp_event_cb(SENSORDPLIB_STATUS_E event) {
    switch (event) {
    case SENSORDPLIB_STATUS_XDMA_FRAME_READY:
        if (_drv_dp_event_cb_on_frame_ready != NULL) {
            _drv_dp_event_cb_on_frame_ready();
        }
        // safe to work 4294967295 / (1 * 30 * 60 * 60 *24 * 365) > 4.5 years in 30fps mode
        if (++_frame_count > SKIP_FRAME_COUNT) {
            _frame.timestamp = _jpeg.timestamp = ma_get_time_ms();
            _frame_ready                       = true;
        } else {
            _frame_ready = false;
            sensordplib_retrigger_capture();
        }
        break;

    default:
        _initiated_before = false;
        MA_LOGE(MA_TAG, "Bad sensor event: %d", event);
    }
}

ma_err_t drv_set_reg(uint16_t addr, uint8_t value) {
    int ec = hx_drv_cis_set_reg(addr, value, 1);
    if (ec != HX_CIS_NO_ERROR) {
        MA_LOGE(MA_TAG, "Set Reg %x to %x Fail: %d", addr, value, ec);
        return MA_EIO;
    }
    return MA_OK;
}

ma_err_t drv_get_reg(uint16_t addr, uint8_t* value) {
    int ec = hx_drv_cis_get_reg(addr, value);
    if (ec != HX_CIS_NO_ERROR) {
        MA_LOGE(MA_TAG, "Get Reg %x Fail: %d", addr, ec);
        return MA_EIO;
    }
    return MA_OK;
}

ma_err_t drv_capture(uint64_t timeout) {
    uint64_t start = ma_get_time_ms();

    while (!_frame_ready) {
        if (ma_get_time_ms() - start >= timeout) {
            _initiated_before = false;
            MA_LOGD(MA_TAG, "Wait frame ready timeout");

            return MA_ETIMEOUT;
        }
        ma_sleep(WAIT_FRAME_YIELD_MS);
    }

    return MA_OK;
}

ma_err_t drv_capture_next() {
    if (_drv_dp_on_next_stream != NULL) {
        _drv_dp_on_next_stream();
    }

    _frame_ready = false;
    sensordplib_retrigger_capture();

    return MA_OK;
}

ma_img_t drv_get_frame() {
    hx_InvalidateDCache_by_Addr((volatile void*)_frame.data, _frame.size);

    return _frame;
}

ma_img_t drv_get_jpeg() {
    uint8_t  frame_no  = 0;
    uint8_t  buffer_no = 0;
    uint32_t reg_val   = 0;
    uint32_t mem_val   = 0;

    if (hx_drv_xdma_get_WDMA2_bufferNo(&buffer_no) || hx_drv_xdma_get_WDMA2NextFrameIdx(&frame_no)) {
        MA_LOGE(MA_TAG, "Get WDMA2 buffer number failed");
    }
    frame_no = frame_no ? frame_no - 1 : buffer_no - 1;

    if (hx_drv_jpeg_get_EncOutRealMEMSize(&reg_val) ||
        hx_drv_jpeg_get_FillFileSizeToMem(frame_no, (uint32_t)_jpegsize_baseaddr, &mem_val) ||
        hx_drv_jpeg_get_MemAddrByFrameNo(frame_no, _wdma2_baseaddr, &_jpeg.data)) {
        MA_LOGE(MA_TAG, "Get JPEG real memory size failed");
    }
    _jpeg.size = mem_val == reg_val ? mem_val : reg_val;

    hx_InvalidateDCache_by_Addr((volatile void*)_jpeg.data, _jpeg.size);

    return _jpeg;
}
