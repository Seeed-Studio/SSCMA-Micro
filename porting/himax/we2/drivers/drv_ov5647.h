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

#ifndef _DRV_OV5647_H_
#define _DRV_OV5647_H_

/* MCU */
#include <WE2_core.h>
#include <ctype.h>
#include <hx_drv_CIS_common.h>
#include <hx_drv_hxautoi2c_mst.h>
#include <hx_drv_scu.h>
#include <hx_drv_scu_export.h>
#include <hx_drv_timer.h>
#include <sensor_dp_lib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "porting/el_misc.h"

#define OV5647_MIPI_640X480   0
#define OV5647_MIPI_2592X1944 1
#define OV5647_MIPI_1296X972  2

#define OV5647_MIPI_MODE      OV5647_MIPI_1296X972

#if (OV5647_MIPI_MODE == OV5647_MIPI_640X480)
    #define OV5647_SENSOR_WIDTH  640
    #define OV5647_SENSOR_HEIGHT 480
    #define OV5647_SUB_SAMPLE    INP_SUBSAMPLE_DISABLE
    #define OV5647_BINNING_0     INP_BINNING_DISABLE
    #define OV5647_BINNING_1     INP_BINNING_4TO2_B
    #define OV5647_BINNING_2     INP_BINNING_8TO2_B
#elif (OV5647_MIPI_MODE == OV5647_MIPI_2592X1944)
    #define OV5647_SENSOR_WIDTH  2592
    #define OV5647_SENSOR_HEIGHT 1944
    #define OV5647_SUB_SAMPLE    INP_SUBSAMPLE_4TO2
    #define OV5647_BINNING_0     INP_BINNING_4TO2_B
    #define OV5647_BINNING_1     INP_BINNING_8TO2_B
    #define OV5647_BINNING_2     INP_BINNING_16TO2_B
#elif (OV5647_MIPI_MODE == OV5647_MIPI_1296X972)
    #define OV5647_SENSOR_WIDTH  1296
    #define OV5647_SENSOR_HEIGHT 972
    #define OV5647_SUB_SAMPLE    INP_SUBSAMPLE_DISABLE
    #define OV5647_BINNING_0     INP_BINNING_4TO2_B
    #define OV5647_BINNING_1     INP_BINNING_8TO2_B
    #define OV5647_BINNING_2     INP_BINNING_16TO2_B
#else
    #error "OV5647_MIPI_MODE not defined"
#endif

#define OV5647_SENSOR_I2CID       (0x36)
#define OV5647_MIPI_CLOCK_FEQ     (220)  // MHz
#define OV5647_MIPI_LANE_CNT      (2)
#define OV5647_MIPI_DPP           (10)    // depth per pixel
#define OV5647_MIPITX_CNTCLK_EN   (1)     // continuous clock output enable
#define OV5647_MIPI_CTRL_OFF      (0x01)  // MIPI OFF
#define OV5647_MIPI_CTRL_ON       (0x14)  // MIPI ON
#define OV5647_LANE_NB            (2)

#define SENSORDPLIB_SENSOR_OV5647 (SENSORDPLIB_SENSOR_HM2130)

#ifdef __cplusplus
extern "C" {
#endif

el_err_code_t drv_ov5647_probe();
el_err_code_t drv_ov5647_init(uint16_t width, uint16_t height);
el_err_code_t drv_ov5647_deinit();

#ifdef __cplusplus
}
#endif

#endif
