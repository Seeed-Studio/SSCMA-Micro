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

#ifndef _EL_CAMERA_HIMAX_H_
#define _EL_CAMERA_HIMAX_H_

#include <logger.h>
#include <sensor_core.h>

#include "core/el_types.h"
#include "porting/el_camera.h"
#include "porting/himax/el_board_config.h"

namespace edgelab {

class CameraHimax : public Camera {
   public:
    CameraHimax()  = default;
    ~CameraHimax() = default;

    el_err_code_t init(size_t width, size_t height) override;
    el_err_code_t deinit() override;

    el_err_code_t start_stream() override;
    el_err_code_t stop_stream() override;

    el_err_code_t get_frame(el_img_t* img) override;
    el_err_code_t get_jpeg(el_img_t* img);

   private:
    Sensor_Cfg_t config;
};

}  // namespace edgelab

#endif
