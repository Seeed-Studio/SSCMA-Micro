#ifndef _DRV_COMMON_H_
#define _DRV_COMMON_H_

#include <core/el_types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <sensor_dp_lib.h>

#ifdef __cplusplus
}
#endif

#include "drv_shared_cfg.h"

void _reset_all_wdma_buffer();

void _drv_dp_event_cb(SENSORDPLIB_STATUS_E event);

extern volatile bool     _initiated_before;
extern volatile bool     _frame_ready;
extern volatile uint32_t _frame_count;
extern volatile uint32_t _wdma1_baseaddr;
extern volatile uint32_t _wdma2_baseaddr;
extern volatile uint32_t _wdma3_baseaddr;
extern volatile uint32_t _jpegsize_baseaddr;
extern el_img_t          _frame;
extern el_img_t          _jpeg;

el_err_code_t _drv_capture(uint32_t timeout);
el_err_code_t _drv_capture_stop();
el_img_t      _drv_get_frame();
el_img_t      _drv_get_jpeg();

el_res_t _drv_fit_res(uint16_t width, uint16_t height);

#endif