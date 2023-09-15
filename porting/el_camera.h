/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Seeed Technology Co.,Ltd
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

#ifndef _EL_CAMERA_H_
#define _EL_CAMERA_H_

#include <cstddef>
#include <cstdint>

#include "core/el_types.h"

namespace edgelab {

class Camera {
   public:
    Camera() : _is_present(false), _is_streaming(false) {}
    virtual ~Camera() = default;

    virtual el_err_code_t init(size_t width, size_t height) = 0;
    virtual el_err_code_t deinit()                          = 0;

    virtual el_err_code_t start_stream() = 0;
    virtual el_err_code_t stop_stream()  = 0;

    virtual el_err_code_t get_frame(el_img_t* img) = 0;

    operator bool() const { return _is_present; }

    bool is_streaming() const { return _is_streaming; }

   protected:
    bool _is_present;
    bool _is_streaming;
};

}  // namespace edgelab

#endif
