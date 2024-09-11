#ifndef _DRV_COMMON_H_
#define _DRV_COMMON_H_

#include <core/ma_types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

ma_err_t drv_set_reg(uint16_t addr, uint8_t value);
ma_err_t drv_get_reg(uint16_t addr, uint8_t* value);
ma_err_t drv_capture(uint64_t timeout);
ma_err_t drv_capture_next();
ma_img_t drv_get_frame();
ma_img_t drv_get_jpeg();

#ifdef __cplusplus
}
#endif

#endif