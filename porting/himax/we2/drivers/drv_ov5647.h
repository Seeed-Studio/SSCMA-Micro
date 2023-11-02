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

#ifndef DRV_OV5647_H_
#define DRV_OV5647_H_

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/el_debug.h"

/* MCU */
#include <WE2_core.h>
#include <hx_drv_CIS_common.h>
#include <hx_drv_hxautoi2c_mst.h>
#include <hx_drv_scu.h>
#include <hx_drv_scu_export.h>
#include <hx_drv_timer.h>
#include <sensor_dp_lib.h>

#ifdef TRUSTZONE_SEC
    #ifdef IP_INST_NS_csirx
        #define CSIRX_REGS_BASE        BASE_ADDR_MIPI_RX_CTRL
        #define CSIRX_DPHY_REG         (BASE_ADDR_APB_MIPI_RX_PHY + 0x50)
        #define CSIRX_DPHY_TUNCATE_REG (BASE_ADDR_APB_MIPI_RX_PHY + 0x48)
    #else
        #define CSIRX_REGS_BASE        BASE_ADDR_MIPI_RX_CTRL_ALIAS
        #define CSIRX_DPHY_REG         (BASE_ADDR_APB_MIPI_RX_PHY_ALIAS + 0x50)
        #define CSIRX_DPHY_TUNCATE_REG (BASE_ADDR_APB_MIPI_RX_PHY_ALIAS + 0x48)
    #endif
#else
    #ifndef TRUSTZONE
        #define CSIRX_REGS_BASE        BASE_ADDR_MIPI_RX_CTRL_ALIAS
        #define CSIRX_DPHY_REG         (BASE_ADDR_APB_MIPI_RX_PHY_ALIAS + 0x50)
        #define CSIRX_DPHY_TUNCATE_REG (BASE_ADDR_APB_MIPI_RX_PHY_ALIAS + 0x48)
    #else
        #define CSIRX_REGS_BASE        BASE_ADDR_MIPI_RX_CTRL
        #define CSIRX_DPHY_REG         (BASE_ADDR_APB_MIPI_RX_PHY + 0x50)
        #define CSIRX_DPHY_TUNCATE_REG (BASE_ADDR_APB_MIPI_RX_PHY + 0x48)
    #endif
#endif

/* el */
#include "core/el_common.h"

#define DEAULT_XHSUTDOWN_PIN      AON_GPIO2
#define OV5647_MAX_WIDTH          640
#define OV5647_MAX_HEIGHT         480

#define OV5647_SENSOR_I2CID       (0x36)
#define OV5647_MIPI_CLOCK_FEQ     (220)  //MHz
#define OV5647_MIPI_LANE_CNT      (2)
#define OV5647_MIPI_DPP           (10)    //depth per pixel
#define OV5647_MIPITX_CNTCLK_EN   (1)     //continuous clock output enable
#define OV5647_MIPI_CTRL_OFF      (0x01)  //MIPI OFF
#define OV5647_MIPI_CTRL_ON       (0x14)  //MIPI ON
#define OV5647_LANE_NB            (2)

#define CIS_I2C_ID                OV5647_SENSOR_I2CID
#define SENSORDPLIB_SENSOR_OV5647 (SENSORDPLIB_SENSOR_HM2130)

#ifdef __cplusplus
extern "C" {
#endif

el_err_code_t drv_ov5647_init(uint16_t width, uint16_t height);
el_err_code_t drv_ov5647_deinit();
el_err_code_t drv_ov5647_capture(uint32_t timeout);
el_img_t      drv_ov5647_get_frame();
el_img_t      drv_ov5647_get_jpeg();

#ifdef __cplusplus
}
#endif

#endif
