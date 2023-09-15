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

#ifndef _SEEED_XIAO_ESP32S3_BOARD_H_
#define _SEEED_XIAO_ESP32S3_BOARD_H_

#define LED_0_PIN                    GPIO_NUM_21

#define CAMERA_MODULE_NAME           "XIAO-S3"
#define CAMERA_PIN_PWDN              -1
#define CAMERA_PIN_RESET             -1

#define CAMERA_PIN_VSYNC             38
#define CAMERA_PIN_HREF              47
#define CAMERA_PIN_PCLK              13
#define CAMERA_PIN_XCLK              10

#define CAMERA_PIN_SIOD              40
#define CAMERA_PIN_SIOC              39

#define CAMERA_PIN_D0                15
#define CAMERA_PIN_D1                17
#define CAMERA_PIN_D2                18
#define CAMERA_PIN_D3                16
#define CAMERA_PIN_D4                14
#define CAMERA_PIN_D5                12
#define CAMERA_PIN_D6                11
#define CAMERA_PIN_D7                48

#define BOARD_LCD_MOSI               9
#define BOARD_LCD_MISO               8
#define BOARD_LCD_SCK                7
#define BOARD_LCD_CS                 2
#define BOARD_LCD_DC                 4
#define BOARD_LCD_RST                -1
#define BOARD_LCD_BL                 -1
#define BOARD_LCD_PIXEL_CLOCK_HZ     (40 * 1000 * 1000)
#define BOARD_LCD_BK_LIGHT_ON_LEVEL  0
#define BOARD_LCD_BK_LIGHT_OFF_LEVEL !BOARD_LCD_BK_LIGHT_ON_LEVEL
#define BOARD_LCD_H_RES              240
#define BOARD_LCD_V_RES              240
#define BOARD_LCD_CMD_BITS           8
#define BOARD_LCD_PARAM_BITS         8
#define BOARD_LCD_ROTATE             SCR_DIR_BTRL
#define LCD_HOST                     SPI2_HOST
#define BOARD_LCD_CONTROLLER         SCREEN_CONTROLLER_GC9A01

#endif
