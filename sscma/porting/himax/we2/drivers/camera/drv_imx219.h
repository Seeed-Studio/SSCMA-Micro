// /*
//  * The MIT License (MIT)
//  *
//  * Copyright (c) 2023 Hongtai Liu, nullptr (Seeed Technology Inc.)
//  *
//  * Permission is hereby granted, free of charge, to any person obtaining a copy
//  * of this software and associated documentation files (the "Software"), to deal
//  * in the Software without restriction, including without limitation the rights
//  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  * copies of the Software, and to permit persons to whom the Software is
//  * furnished to do so, subject to the following conditions:
//  *
//  * The above copyright notice and this permission notice shall be included in
//  * all copies or substantial portions of the Software.
//  *
//  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  * THE SOFTWARE.
//  *
//  */

// #ifndef _DRV_IMX219_H_
// #define _DRV_IMX219_H_

// /* MCU */
// #include <WE2_core.h>
// #include <ctype.h>
// #include <hx_drv_CIS_common.h>
// #include <hx_drv_hxautoi2c_mst.h>
// #include <hx_drv_scu.h>
// #include <hx_drv_scu_export.h>
// #include <hx_drv_timer.h>
// #include <sensor_dp_lib.h>
// #include <stdarg.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #include "core/el_debug.h"
// #include "core/el_types.h"
// #include "porting/el_misc.h"

// #define IMX219_SENSOR_I2CID       (0x10)
// #define IMX219_MIPI_CLOCK_FEQ     (456)  // MHz
// #define IMX219_MIPI_LANE_CNT      (2)
// #define IMX219_MIPI_DPP           (10)  // depth per pixel
// #define IMX219_MIPITX_CNTCLK_EN   (1)   // continuous clock output enable
// #define IMX219_LANE_NB            (2)

// #define SENSORDPLIB_SENSOR_IMX219 (SENSORDPLIB_SENSOR_HM2130)

// #define IMX219_SENSOR_WIDTH       3280
// #define IMX219_SENSOR_HEIGHT      2464

// #define IMX219_EXPOSURE_DEFAULT   (0x640)
// #define IMX219_EXPOSURE_SETTING   (0xA40)
// #define IMX219_ANA_GAIN_MIN       0
// #define IMX219_ANA_GAIN_MAX       232
// #define IMX219_ANA_GAIN_DEFAULT   0x0
// #define IMX219_AGAIN_SETTING      (0x40)
// #define IMX219_DGTL_GAIN_MIN      0x0100
// #define IMX219_DGTL_GAIN_MAX      0x0fff
// #define IMX219_DGTL_GAIN_DEFAULT  0x0100
// #define IMX219_DGAIN_SETTING      (0x200)

// #define IMX219_REG_BINNING_MODE   0x0174
// #define IMX219_BINNING_NONE       0x0000
// #define IMX219_BINNING_2X2        0x0101
// #define IMX219_BINNING_2X2_ANALOG 0x0303
// #define IMX219_BINNING_SETTING    (IMX219_BINNING_NONE)

// #ifdef __cplusplus
// extern "C" {
// #endif

// el_err_code_t drv_imx219_probe();
// el_err_code_t drv_imx219_init(uint16_t width, uint16_t height);
// el_err_code_t drv_imx219_deinit();

// #ifdef __cplusplus
// }
// #endif

// #endif
