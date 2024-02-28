/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 (Seeed Technology Inc.)
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

#include "el_camera_we1.h"

extern "C" {
#include <camera_core.h>
#include <datapath.h>
}

namespace edgelab {

CameraWE1::CameraWE1() : Camera(0b00000001) {}

el_err_code_t CameraWE1::init(SensorOptIdType opt_id) {
    ERROR_T ret = ERROR_NONE;

    uint16_t width  = 240;
    uint16_t height = 240;

    this->config.sensor_type            = SENSOR_CAMERA;
    this->config.data.camera_cfg.width  = width;
    this->config.data.camera_cfg.height = height;

    ret = datapath_init(this->config.data.camera_cfg.width, this->config.data.camera_cfg.height);
    if (ret != ERROR_NONE) {
        return EL_ELOG;
    }
    ret = sensor_init(&this->config);
    if (ret != ERROR_NONE) {
        return EL_ELOG;
    }

    return EL_OK;
}

el_err_code_t CameraWE1::deinit() {
    camera_deinit();
    return EL_OK;
}

el_err_code_t CameraWE1::start_stream() {
    datapath_start_work();

    while (!datapath_get_img_state())
        ;

    this->_is_streaming = true;
    return EL_OK;
}

el_err_code_t CameraWE1::stop_stream() {
    datapath_stop_work();
    this->_is_streaming = false;
    return EL_OK;
}

el_err_code_t CameraWE1::get_frame(el_img_t* img) {
    if (!this->_is_streaming) {
        return EL_EIO;
    }

    volatile uint32_t yuv422_addr;
    yuv422_addr = datapath_get_yuv_img_addr();

    img->width  = this->config.data.camera_cfg.width;
    img->height = this->config.data.camera_cfg.height;
    img->data   = reinterpret_cast<uint8_t*>(yuv422_addr);
    img->size   = img->width * img->height * 2;
    img->format = EL_PIXEL_FORMAT_YUV422;

    return EL_OK;
}

el_err_code_t CameraWE1::get_processed_frame(el_img_t* img) {
    if (!this->_is_streaming) {
        return EL_EIO;
    }

    volatile uint32_t jpeg_addr;
    volatile uint32_t jpeg_size;
    datapath_get_jpeg_img((uint32_t*)&jpeg_addr, (uint32_t*)&jpeg_size);

    img->width  = this->config.data.camera_cfg.width;
    img->height = this->config.data.camera_cfg.height;
    img->data   = (uint8_t*)jpeg_addr;
    img->size   = jpeg_size;
    img->format = EL_PIXEL_FORMAT_JPEG;

    return EL_OK;
}

}  // namespace edgelab
