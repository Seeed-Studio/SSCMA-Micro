#ifndef _DRV_COMMON_H_
#define _DRV_COMMON_H_

#include <core/ma_types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <sensor_dp_lib.h>
#ifdef __cplusplus
}
#endif

#include "drv_shared_cfg.h"

extern volatile bool     _initiated_before;
extern volatile bool     _frame_ready;
extern volatile uint32_t _frame_count;
extern volatile uint32_t _wdma1_baseaddr;
extern volatile uint32_t _wdma2_baseaddr;
extern volatile uint32_t _wdma3_baseaddr;
extern volatile uint32_t _jpegsize_baseaddr;
extern ma_img_t          _frame;
extern ma_img_t          _jpeg;

extern void (*_drv_dp_event_cb_on_frame_ready)();
extern void (*_drv_dp_on_stop_stream)();

void _reset_all_wdma_buffer();
void _drv_dp_event_cb(SENSORDPLIB_STATUS_E event);

ma_err_t _drv_capture(uint64_t timeout);
ma_err_t _drv_capture_stop();
ma_img_t _drv_get_frame();
ma_img_t _drv_get_jpeg();

#endif