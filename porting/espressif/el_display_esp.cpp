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

#include "el_display_esp.h"

#include "core/el_debug.h"
#include "porting/espressif/el_board_config.h"

namespace edgelab {

el_err_code_t DisplayEsp::init() {
    spi_config_t bus_conf = {
      .miso_io_num     = static_cast<gpio_num_t>(BOARD_LCD_MISO),
      .mosi_io_num     = static_cast<gpio_num_t>(BOARD_LCD_MOSI),
      .sclk_io_num     = static_cast<gpio_num_t>(BOARD_LCD_SCK),
      .max_transfer_sz = 2 * BOARD_LCD_H_RES * BOARD_LCD_V_RES + 10,
    };

    spi_bus_handle_t spi_bus = spi_bus_create(LCD_HOST, &bus_conf);

    scr_interface_spi_config_t spi_lcd_cfg = {
      .spi_bus    = spi_bus,
      .pin_num_cs = BOARD_LCD_CS,
      .pin_num_dc = BOARD_LCD_DC,
      .clk_freq   = BOARD_LCD_PIXEL_CLOCK_HZ,
      .swap_data  = 0,
    };

    scr_interface_driver_t* iface_drv;
    scr_interface_create(SCREEN_IFACE_SPI, &spi_lcd_cfg, &iface_drv);

    esp_err_t ret = scr_find_driver(BOARD_LCD_CONTROLLER, &_lcd);

    if (ESP_OK != ret) {
        return EL_EIO;
        EL_ELOG("[Screen] find deiver failed");
    }

    scr_controller_config_t lcd_cfg = {
      .interface_drv     = iface_drv,
      .pin_num_rst       = static_cast<gpio_num_t>(BOARD_LCD_RST),
      .pin_num_bckl      = static_cast<gpio_num_t>(BOARD_LCD_BL),
      .rst_active_level  = 0,
      .bckl_active_level = 0,
      .width             = BOARD_LCD_H_RES,
      .height            = BOARD_LCD_V_RES,
      .offset_hor        = 0,
      .offset_ver        = 0,
      .rotate            = static_cast<scr_dir_t>(BOARD_LCD_ROTATE),
    };

    ret = _lcd.init(&lcd_cfg);

    if (ESP_OK != ret) {
        return EL_EIO;
        EL_ELOG("[Screen] initialize failed");
    }

    _lcd.get_info(&_lcd_info);

    LOG_D("[Screen] name:%s | width:%d | height:%d", _lcd_info.name, _lcd_info.width, _lcd_info.height);

    this->_is_present = true;
    return EL_OK;
}

el_err_code_t DisplayEsp::deinit() {
    esp_err_t ret = _lcd.deinit();
    if (ESP_OK != ret) {
        return EL_EIO;
        EL_ELOG("[Screen] deinitialize failed");
    }
    this->_is_present = false;
    return EL_OK;
}

el_err_code_t DisplayEsp::show(const el_img_t* img) {
    esp_err_t ret = _lcd.draw_bitmap(0, 0, img->width, img->height, (uint16_t*)(img->data));
    if (ESP_OK != ret) {
        return EL_EIO;
        EL_ELOG("[Screen] show failed");
    }
    return EL_OK;
}

}  // namespace edgelab
