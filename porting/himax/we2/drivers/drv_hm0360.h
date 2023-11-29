/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu, nullptr (Seeed Technology Inc.)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _DRV_HM0360_H_
#define _DRV_HM0360_H_

/* MCU */
#include <WE2_device.h>
#include <ctype.h>
#include <hx_drv_CIS_common.h>
#include <hx_drv_scu.h>
#include <sensor_dp_lib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "porting/el_misc.h"

#define DEAULT_XHSUTDOWN_PIN AON_GPIO2
#define HM0360_MAX_WIDTH     640
#define HM0360_MAX_HEIGHT    480

#ifdef __cplusplus
extern "C" {
#endif

el_err_code_t drv_hm0360_init(uint16_t width, uint16_t height);
el_err_code_t drv_hm0360_deinit();
el_err_code_t drv_hm0360_capture(uint32_t timeout);
el_img_t      drv_hm0360_get_frame();
el_img_t      drv_hm0360_get_jpeg();

#ifdef __cplusplus
}
#endif

#endif
