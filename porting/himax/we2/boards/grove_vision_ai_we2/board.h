/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu (Seeed Technology Inc.)
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

#ifndef _BOARD_GROVE_VISION_AI_WE2_H_
#define _BOARD_GROVE_VISION_AI_WE2_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <BITOPS.h>
#include <WE2_core.h>
#include <WE2_device.h>
#include <board.h>
#include <hx_drv_pmu.h>
#include <hx_drv_pmu_export.h>
#include <powermode.h>
#include <powermode_export.h>
#include <xprintf.h>

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

#include <ethosu_driver.h>
#include <spi_eeprom_comm.h>

#ifdef __cplusplus
}
#endif

#endif
