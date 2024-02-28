/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 nullptr (Seeed Technology Inc.)
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

#include "el_camera_we2.h"

extern "C" {
#include <drivers/drv_ov5647.h>
#include <sensor_dp_lib.h>
}

namespace edgelab {

CameraWE2::CameraWE2() : Camera(0b00000111) {}

el_err_code_t CameraWE2::init(SensorOptIdType opt_id) {
    if (this->_is_present) [[unlikely]] {
        return EL_OK;
    }

    auto ret = EL_OK;
    switch (opt_id) {
    case 0:
        ret = drv_ov5647_init(240, 240);
        this->_current_opt_id = 0;
        break;
    case 1:
        ret = drv_ov5647_init(480, 480);
        this->_current_opt_id = 1;
        break;
    case 2:
        ret = drv_ov5647_init(640, 480);
        this->_current_opt_id = 2;
        break;
    default:
        ret = EL_EINVAL;
    }

    if (ret == EL_OK) [[likely]] {
        this->_is_present = true;
    }

    return ret;
}

el_err_code_t CameraWE2::deinit() {
    if (!this->_is_present) [[unlikely]] {
        return EL_OK;
    }


    auto ret = drv_ov5647_deinit();

    if (ret == EL_OK) [[likely]] {
        this->_is_present = false;
    }

    return ret;
}

el_err_code_t CameraWE2::start_stream() {
    if (this->_is_streaming) [[unlikely]] {
        return EL_EBUSY;
    } else {
        if (!this->_is_present) [[unlikely]] {
            return EL_EPERM;
        }
    }

    auto ret = drv_ov5647_capture(2000);

    if (ret == EL_OK) [[likely]] {
        this->_is_streaming = true;
    }

    return ret;
}

el_err_code_t CameraWE2::stop_stream() {
    if (!this->_is_streaming) [[unlikely]] {
        return EL_OK;  // only ensure the camera is not streaming
    }

    auto ret = drv_ov5647_capture_stop();

    if (ret == EL_OK) [[likely]] {
        this->_is_streaming = false;
    }

    return ret;
}

el_err_code_t CameraWE2::get_frame(el_img_t* img) {
    if (!this->_is_streaming) [[unlikely]] {
        return EL_EPERM;
    }

    *img = drv_ov5647_get_frame();
    // just assign, not sure whether the img is valid

    return EL_OK;
}

el_err_code_t CameraWE2::get_processed_frame(el_img_t* img) {
    if (!this->_is_streaming) [[unlikely]] {
        return EL_EPERM;
    }

    *img = drv_ov5647_get_jpeg();
    // just assign, not sure whether the img is valid

    return EL_OK;
}

}  // namespace edgelab
