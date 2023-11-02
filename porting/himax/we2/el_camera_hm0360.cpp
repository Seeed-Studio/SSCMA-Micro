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

#include "el_camera_hm0360.h"

extern "C" {
#include <drivers/drv_hm0360.h>
#include <sensor_dp_lib.h>
}

namespace edgelab {

el_err_code_t CameraHM0360::init(size_t width, size_t height) {
    EL_ASSERT(!this->_is_present);
    auto ret          = drv_hm0360_init(width, height);
    this->_is_present = ret == EL_OK;
    return ret;
}

el_err_code_t CameraHM0360::deinit() {
    auto ret          = drv_hm0360_deinit();
    this->_is_present = false;
    return ret;
}

el_err_code_t CameraHM0360::start_stream() {
    this->_is_streaming = true;
    return drv_hm0360_capture(2000);
}

el_err_code_t CameraHM0360::stop_stream() {
    this->_is_streaming = false;
    return EL_OK;
}

el_err_code_t CameraHM0360::get_frame(el_img_t* img) {
    if (!this->_is_streaming) {
        return EL_EIO;
    }
    *img = drv_hm0360_get_frame();
    el_printf("frame:%08x, size: %d\n", img->data, img->size);
    return EL_OK;
}

el_err_code_t CameraHM0360::get_processed_frame(el_img_t* img) {
    if (!this->_is_streaming) {
        return EL_EIO;
    }
    *img = drv_hm0360_get_jpeg();
    el_printf("jpeg:%08x, size: %d\n", img->data, img->size);
    return EL_OK;
}

}  // namespace edgelab
