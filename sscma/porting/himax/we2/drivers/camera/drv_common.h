#ifndef _DRV_COMMON_H_
#define _DRV_COMMON_H_

#include <core/ma_types.h>
#include <stdint.h>

ma_err_t drv_capture(uint64_t timeout);
ma_err_t drv_capture_next();
ma_img_t drv_get_frame();
ma_img_t drv_get_jpeg();

#endif