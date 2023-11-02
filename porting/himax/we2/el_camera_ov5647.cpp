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

#include "el_camera_ov5647.h"

#include <drivers/drv_ov5647.h>
#include <sensor_dp_lib.h>

namespace edgelab {

el_err_code_t CameraOV5647::init(size_t width, size_t height) { return drv_ov5647_init(width, height); }

el_err_code_t CameraOV5647::deinit() { return drv_ov5647_deinit(); }

el_err_code_t CameraOV5647::start_stream() {
    this->_is_streaming = true;
    return drv_ov5647_capture(2000);
}

el_err_code_t CameraOV5647::stop_stream() {
    this->_is_streaming = false;
    return EL_OK;
}

el_err_code_t CameraOV5647::get_frame(el_img_t* img) {
    if (!this->_is_streaming) {
        return EL_EIO;
    }
    *img = drv_ov5647_get_frame();
    return EL_OK;
}

el_err_code_t CameraOV5647::get_processed_frame(el_img_t* img) {
    if (!this->_is_streaming) {
        return EL_EIO;
    }
    *img = drv_ov5647_get_jpeg();
    return EL_OK;
}

}  // namespace edgelab
