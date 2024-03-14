/*
 * The MIT License (MIT)
 *
 * Copyright (c) Seeed Technology Inc.
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

#ifndef _BOARD_DEV_BOARD_WE2_H_
#define _BOARD_DEV_BOARD_WE2_H_

#define PRODUCT_NAME_PREFIX "dev_board"
#define PRODUCT_NAME_SUFFIX "v1"
#define DEVICE_NAME         (PRODUCT_NAME_PREFIX "_" PRODUCT_NAME_SUFFIX)
#define PORT_DEVICE_NAME    "Dev Board"

#ifdef __cplusplus
extern "C" {
#endif

#include <WE2_device.h>
#include <hx_drv_gpio.h>
#include <hx_drv_scu.h>
#include <hx_drv_scu_export.h>

#ifdef TRUSTZONE_SEC
    #if (__ARM_FEATURE_CMSE & 1) == 0
        #error "Need ARMv8-M security extensions"
    #elif (__ARM_FEATURE_CMSE & 2) == 0
        #error "Compile with --cmse"
    #endif

    #include <arm_cmse.h>

    #ifdef NSC
        #include <veneer_table.h>
    #endif
    /* Trustzone config. */

    #ifndef TRUSTZONE_SEC_ONLY
        /* FreeRTOS includes. */
        #include <secure_port_macros.h>
    #endif
#endif

#define CONFIG_EL_CAMERA_PWR_CTRL_INIT_F                        \
    {                                                           \
        hx_drv_gpio_set_output(AON_GPIO1, GPIO_OUT_LOW);        \
        hx_drv_scu_set_PA1_pinmux(SCU_PA1_PINMUX_AON_GPIO1, 0); \
        hx_drv_gpio_set_out_value(AON_GPIO1, GPIO_OUT_LOW);     \
    }

#define CONFIG_EL_SPI_CS_INIT_F \
    { hx_drv_scu_set_PB5_pinmux(SCU_PB5_PINMUX_SPI_S_CS, 1); }
#define CONFIG_EL_SPI_CTRL_INIT_F \
    { hx_drv_scu_set_PB8_pinmux(SCU_PB8_PINMUX_GPIO2_1, 1); }
#define CONFIG_EL_SPI_CTRL_PIN GPIO21

#ifdef __cplusplus
}
#endif

#endif
