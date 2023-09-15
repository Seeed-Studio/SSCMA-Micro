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

#ifndef _EL_CV_H_
#define _EL_CV_H_

#include <cstdint>

#include "core/el_types.h"

namespace edgelab {

el_err_code_t el_img_convert(const el_img_t* src, el_img_t* dst);

void el_draw_rect(el_img_t* img, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color, uint8_t thickness = 1);

void el_fill_rect(el_img_t* img, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color);

void el_draw_point(el_img_t* img, int16_t x, int16_t y, uint32_t color);

void el_draw_h_line(el_img_t* img, int16_t x0, int16_t x1, int16_t y, uint32_t color);

void el_draw_v_line(el_img_t* img, int16_t x, int16_t y0, int16_t y1, uint32_t color);

}  // namespace edgelab

#endif
