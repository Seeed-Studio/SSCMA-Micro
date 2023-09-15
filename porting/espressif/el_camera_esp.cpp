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

#include "el_camera_esp.h"

#include "core/el_debug.h"

namespace edgelab {

el_err_code_t CameraEsp::init(size_t width, size_t height) {
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = CAMERA_PIN_D0;
    config.pin_d1       = CAMERA_PIN_D1;
    config.pin_d2       = CAMERA_PIN_D2;
    config.pin_d3       = CAMERA_PIN_D3;
    config.pin_d4       = CAMERA_PIN_D4;
    config.pin_d5       = CAMERA_PIN_D5;
    config.pin_d6       = CAMERA_PIN_D6;
    config.pin_d7       = CAMERA_PIN_D7;
    config.pin_xclk     = CAMERA_PIN_XCLK;
    config.pin_pclk     = CAMERA_PIN_PCLK;
    config.pin_vsync    = CAMERA_PIN_VSYNC;
    config.pin_href     = CAMERA_PIN_HREF;
    config.pin_sscb_sda = CAMERA_PIN_SIOD;
    config.pin_sscb_scl = CAMERA_PIN_SIOC;
    config.pin_pwdn     = CAMERA_PIN_PWDN;
    config.pin_reset    = CAMERA_PIN_RESET;
    config.xclk_freq_hz = XCLK_FREQ_HZ;
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size   = FRAMESIZE_240X240;
    config.jpeg_quality = 12;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        EL_ELOG("[Camera] init failed with error 0x%x", err);
        return EL_EIO;
    }

    sensor = esp_camera_sensor_get();
    sensor->set_vflip(sensor, 1);  // flip it back
    sensor->set_hmirror(sensor, 1);
    // initial sensors are flipped vertically and colors are a bit saturated
    if (sensor->id.PID == OV3660_PID) {
        sensor->set_brightness(sensor, 1);   // up the blightness just a bit
        sensor->set_saturation(sensor, -2);  // lower the saturation
    }

    this->_is_present = true;

    return EL_OK;
}

el_err_code_t CameraEsp::deinit() {
    this->_is_present = !(esp_camera_deinit() == ESP_OK);
    return this->_is_present ? EL_AGAIN : EL_OK;
}

el_err_code_t CameraEsp::start_stream() {
    fb = esp_camera_fb_get();
    if (!fb) {
        EL_ELOG("[Camera] capture failed");
        return EL_EIO;
    }
    this->_is_streaming = true;
    return EL_OK;
}

el_err_code_t CameraEsp::stop_stream() {
    if (this->_is_streaming) [[likely]] {
        esp_camera_fb_return(fb);
        this->_is_streaming = false;
        return EL_OK;
    }
    return EL_ELOG;
}

el_err_code_t CameraEsp::get_frame(el_img_t* img) {
    if (!this->_is_streaming) {
        return EL_EIO;
    }
    if (!fb) {
        EL_ELOG("[Camera] capture failed");
        this->_is_streaming = false;
        return EL_EIO;
    }
    img->width  = fb->width;
    img->height = fb->height;
    img->data   = fb->buf;
    img->size   = fb->len;
    img->format = EL_PIXEL_FORMAT_RGB565;
    return EL_OK;
}

framesize_t CameraEsp::fit_resolution(size_t width, size_t height) {
    framesize_t res = FRAMESIZE_INVALID;
    return res;
}

}  // namespace edgelab
