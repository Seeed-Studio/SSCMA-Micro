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

// #ifndef _DRV_IMX708_H_
// #define _DRV_IMX708_H_

// /* MCU */
// #include <WE2_core.h>
// #include <ctype.h>
// #include <hx_drv_CIS_common.h>
// #include <hx_drv_hxautoi2c_mst.h>
// #include <hx_drv_inp.h>
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

// // ------------------------------------------------------------------------

// #define IMX708_SENSOR_WIDTH           2304
// #define IMX708_SENSOR_HEIGHT          1296

// #define IMX708_QBC_ADJUST             (0x02)
// #define IMX708_REG_BASE_SPC_GAINS_L   (0x7b10)
// #define IMX708_REG_BASE_SPC_GAINS_R   (0x7c00)
// #define IMX708_LPF_INTENSITY_EN       (0xC428)
// #define IMX708_LPF_INTENSITY_ENABLED  (0x00)
// #define IMX708_LPF_INTENSITY_DISABLED (0x01)
// #define IMX708_LPF_INTENSITY          (0xC429)
// #define IMX708_REG_EXPOSURE           (0x0202)
// #define IMX708_EXPOSURE_DEFAULT       (0x640)
// #define IMX708_EXPOSURE_SETTING       (0x0A40)

// #define IMX708_SENSOR_I2CID           (0x1A)
// #define IMX708_MIPI_CLOCK_FEQ         (450)  // MHz
// #define IMX708_MIPI_LANE_CNT          (2)
// #define IMX708_MIPI_DPP               (10)  // depth per pixel
// #define IMX708_MIPITX_CNTCLK_EN       (1)   // continuous clock output enable
// #define IMX708_LANE_NB                (2)

// #define IMX708_SUB_SAMPLE             INP_SUBSAMPLE_DISABLE

// #define IMX708_BINNING_0              INP_BINNING_4TO2_B
// #define IMX708_BINNING_1              INP_BINNING_8TO2_B
// #define IMX708_BINNING_2              INP_BINNING_16TO2_B

// // ------------------------------------------------------------------------

// #ifdef __cplusplus
// extern "C" {
// #endif

// el_err_code_t drv_imx708_probe();
// el_err_code_t drv_imx708_init(uint16_t width, uint16_t height);
// el_err_code_t drv_imx708_deinit();

// #ifdef __cplusplus
// }
// #endif

// #endif
