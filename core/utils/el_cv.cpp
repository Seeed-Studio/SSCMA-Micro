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

#include "el_cv.h"

#include <memory>

#include "core/el_common.h"
#include "core/el_compiler.h"
#include "core/el_debug.h"
#include "core/el_types.h"

#ifdef CONFIG_EL_LIB_JPEGENC
    #include "third_party/JPEGENC/JPEGENC.h"
#endif

namespace edgelab {

namespace constants {

const static uint8_t RGB565_TO_RGB888_LOOKUP_TABLE_5[] = {
  0x00, 0x08, 0x10, 0x19, 0x21, 0x29, 0x31, 0x3A, 0x42, 0x4A, 0x52, 0x5A, 0x63, 0x6B, 0x73, 0x7B,
  0x84, 0x8C, 0x94, 0x9C, 0xA5, 0xAD, 0xB5, 0xBD, 0xC5, 0xCE, 0xD6, 0xDE, 0xE6, 0xEF, 0xF7, 0xFF,
};

const static uint8_t RGB565_TO_RGB888_LOOKUP_TABLE_6[] = {
  0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2D, 0x31, 0x35, 0x39, 0x3D,
  0x41, 0x45, 0x49, 0x4D, 0x51, 0x55, 0x59, 0x5D, 0x61, 0x65, 0x69, 0x6D, 0x71, 0x75, 0x79, 0x7D,
  0x82, 0x86, 0x8A, 0x8E, 0x92, 0x96, 0x9A, 0x9E, 0xA2, 0xA6, 0xAA, 0xAE, 0xB2, 0xB6, 0xBA, 0xBE,
  0xC2, 0xC6, 0xCA, 0xCE, 0xD2, 0xD7, 0xDB, 0xDF, 0xE3, 0xE7, 0xEB, 0xEF, 0xF3, 0xF7, 0xFB, 0xFF,
};

}  // namespace constants

namespace types {

typedef struct EL_ATTR_PACKED b16_t {
    uint8_t b0_8;
    uint8_t b8_16;
} b16_t;

typedef struct EL_ATTR_PACKED b24_t {
    uint8_t b0_8;
    uint8_t b8_16;
    uint8_t b16_24;
} b24_t;

}  // namespace types

using namespace constants;
using namespace types;

// TODO: need to be optimized
EL_ATTR_WEAK void yuv422p_to_rgb(const el_img_t* src, el_img_t* dst) {
    int32_t  y;
    int32_t  cr;
    int32_t  cb;
    int32_t  r, g, b;
    uint32_t init_index, cbcr_index, index;
    uint32_t u_chunk = src->width * src->height;
    uint32_t v_chunk = src->width * src->height + src->width * src->height / 2;
    float    beta_h = (float)src->height / dst->height, beta_w = (float)src->width / dst->width;

    EL_ASSERT(src->format == EL_PIXEL_FORMAT_YUV422);

    for (int i = 0; i < dst->height; i++) {
        for (int j = 0; j < dst->width; j++) {
            int tmph = i * beta_h, tmpw = beta_w * j;
            index      = i * dst->width + j;
            init_index = tmph * src->width + tmpw;
            cbcr_index = init_index % 2 ? init_index - 1 : init_index;

            y  = src->data[init_index];
            cb = src->data[u_chunk + cbcr_index / 2];
            cr = src->data[v_chunk + cbcr_index / 2];
            r  = (int32_t)(y + (14065 * (cr - 128)) / 10000);
            g  = (int32_t)(y - (3455 * (cb - 128)) / 10000 - (7169 * (cr - 128)) / 10000);
            b  = (int32_t)(y + (17790 * (cb - 128)) / 10000);

            switch (dst->rotate) {
            case EL_PIXEL_ROTATE_90:
                index = (index % dst->width) * (dst->height) + (dst->height - 1 - index / dst->width);
                break;
            case EL_PIXEL_ROTATE_180:
                index = (dst->width - 1 - index % dst->width) + (dst->height - 1 - index / dst->width) * (dst->width);
                break;
            case EL_PIXEL_ROTATE_270:
                index = (dst->width - 1 - index % dst->width) * (dst->height) + index / dst->width;
                break;
            default:
                break;
            }
            if (dst->format == EL_PIXEL_FORMAT_GRAYSCALE) {
                uint8_t gray     = (r * 299 + g * 587 + b * 114) / 1000;
                dst->data[index] = (uint8_t)EL_CLIP(gray, 0, 255);
            } else if (dst->format == EL_PIXEL_FORMAT_RGB565) {
                dst->data[index * 2 + 0] = (r & 0xF8) | (g >> 5);
                dst->data[index * 2 + 1] = ((g << 3) & 0xE0) | (b >> 3);
            } else if (dst->format == EL_PIXEL_FORMAT_RGB888) {
                dst->data[index * 3 + 0] = (uint8_t)EL_CLIP(r, 0, 255);
                dst->data[index * 3 + 1] = (uint8_t)EL_CLIP(g, 0, 255);
                dst->data[index * 3 + 2] = (uint8_t)EL_CLIP(b, 0, 255);
            }
        }
    }
}

EL_ATTR_WEAK void rgb888_to_rgb888(const el_img_t* src, el_img_t* dst) {
    uint16_t sw = src->width;
    uint16_t sh = src->height;
    uint16_t dw = dst->width;
    uint16_t dh = dst->height;

    uint32_t beta_w = (sw << 16) / dw;
    uint32_t beta_h = (sh << 16) / dh;

    uint32_t i_mul_bh_sw = 0;

    uint32_t init_index = 0;
    uint32_t index      = 0;

    const uint8_t* src_p = src->data;
    uint8_t*       dst_p = dst->data;

    switch (dst->rotate) {
    case EL_PIXEL_ROTATE_90:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                  *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
            }
        }
        break;

    case EL_PIXEL_ROTATE_180:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                  *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
            }
        }
        break;

    case EL_PIXEL_ROTATE_270:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                  *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
            }
        }
        break;

    default:
        memcpy(dst_p, src_p, dst->size < src->size ? dst->size : src->size);
    }
}

EL_ATTR_WEAK void rgb888_to_rgb565(const el_img_t* src, el_img_t* dst) {
    uint16_t sw = src->width;
    uint16_t sh = src->height;
    uint16_t dw = dst->width;
    uint16_t dh = dst->height;

    uint32_t beta_w = (sw << 16) / dw;
    uint32_t beta_h = (sh << 16) / dh;

    uint32_t i_mul_bh_sw = 0;
    uint32_t i_mul_dw    = 0;

    uint32_t init_index = 0;
    uint32_t index      = 0;

    const uint8_t* src_p = src->data;
    uint8_t*       dst_p = dst->data;

    b24_t   b24{};
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    switch (dst->rotate) {
    case EL_PIXEL_ROTATE_90:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) | ((b24.b8_16 & 0xE0) >> 5)];

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  b16_t{.b0_8  = static_cast<uint8_t>((r & 0xF8) | (g >> 5)),
                        .b8_16 = static_cast<uint8_t>(((g << 3) & 0xE0) | (b >> 3))};
            }
        }
        break;

    case EL_PIXEL_ROTATE_180:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) | ((b24.b8_16 & 0xE0) >> 5)];

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  b16_t{.b0_8  = static_cast<uint8_t>((r & 0xF8) | (g >> 5)),
                        .b8_16 = static_cast<uint8_t>(((g << 3) & 0xE0) | (b >> 3))};
            }
        }
        break;

    case EL_PIXEL_ROTATE_270:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) | ((b24.b8_16 & 0xE0) >> 5)];

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  b16_t{.b0_8  = static_cast<uint8_t>((r & 0xF8) | (g >> 5)),
                        .b8_16 = static_cast<uint8_t>(((g << 3) & 0xE0) | (b >> 3))};
            }
        }
        break;

    default:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;
            i_mul_dw    = i * dw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = j + i_mul_dw;

                b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) | ((b24.b8_16 & 0xE0) >> 5)];

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  b16_t{.b0_8  = static_cast<uint8_t>((r & 0xF8) | (g >> 5)),
                        .b8_16 = static_cast<uint8_t>(((g << 3) & 0xE0) | (b >> 3))};
            }
        }
    }
}

EL_ATTR_WEAK void rgb888_to_gray(const el_img_t* src, el_img_t* dst) {
    uint16_t sw = src->width;
    uint16_t sh = src->height;
    uint16_t dw = dst->width;
    uint16_t dh = dst->height;

    uint32_t beta_w = (sw << 16) / dw;
    uint32_t beta_h = (sh << 16) / dh;

    uint32_t i_mul_bh_sw = 0;
    uint32_t i_mul_dw    = 0;

    uint32_t init_index = 0;
    uint32_t index      = 0;

    const uint8_t* src_p = src->data;
    uint8_t*       dst_p = dst->data;

    b24_t b24{};

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    switch (dst->rotate) {
    case EL_PIXEL_ROTATE_90:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) | ((b24.b8_16 & 0xE0) >> 5)];

                dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
            }
        }
        break;

    case EL_PIXEL_ROTATE_180:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) | ((b24.b8_16 & 0xE0) >> 5)];

                dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
            }
        }
        break;

    case EL_PIXEL_ROTATE_270:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) | ((b24.b8_16 & 0xE0) >> 5)];

                dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
            }
        }
        break;

    default:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;
            i_mul_dw    = i * dw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = j + i_mul_dw;

                b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) | ((b24.b8_16 & 0xE0) >> 5)];

                dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
            }
        }
    }
}

EL_ATTR_WEAK void rgb565_to_rgb888(const el_img_t* src, el_img_t* dst) {
    uint16_t sw = src->width;
    uint16_t sh = src->height;
    uint16_t dw = dst->width;
    uint16_t dh = dst->height;

    uint32_t beta_w = (sw << 16) / dw;
    uint32_t beta_h = (sh << 16) / dh;

    uint32_t i_mul_bh_sw = 0;
    uint32_t i_mul_dw    = 0;

    uint32_t init_index = 0;
    uint32_t index      = 0;

    const uint8_t* src_p = src->data;
    uint8_t*       dst_p = dst->data;

    b16_t b16{};

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    switch (dst->rotate) {
    case EL_PIXEL_ROTATE_90:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) | ((b16.b8_16 & 0xE0) >> 5)];

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) = b24_t{.b0_8 = r, .b8_16 = g, .b16_24 = b};
            }
        }
        break;

    case EL_PIXEL_ROTATE_180:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) | ((b16.b8_16 & 0xE0) >> 5)];

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) = b24_t{.b0_8 = r, .b8_16 = g, .b16_24 = b};
            }
        }
        break;

    case EL_PIXEL_ROTATE_270:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) | ((b16.b8_16 & 0xE0) >> 5)];

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) = b24_t{.b0_8 = r, .b8_16 = g, .b16_24 = b};
            }
        }
        break;

    default:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;
            i_mul_dw    = i * dw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = j + i_mul_dw;

                b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) | ((b16.b8_16 & 0xE0) >> 5)];

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) = b24_t{.b0_8 = r, .b8_16 = g, .b16_24 = b};
            }
        }
    }
}

EL_ATTR_WEAK void rgb565_to_rgb565(const el_img_t* src, el_img_t* dst) {
    uint16_t sw = src->width;
    uint16_t sh = src->height;
    uint16_t dw = dst->width;
    uint16_t dh = dst->height;

    uint32_t beta_w = (sw << 16) / dw;
    uint32_t beta_h = (sh << 16) / dh;

    uint32_t i_mul_bh_sw = 0;

    uint32_t init_index = 0;
    uint32_t index      = 0;

    const uint8_t* src_p = src->data;
    uint8_t*       dst_p = dst->data;

    switch (dst->rotate) {
    case EL_PIXEL_ROTATE_90:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
            }
        }
        break;

    case EL_PIXEL_ROTATE_180:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
            }
        }
        break;

    case EL_PIXEL_ROTATE_270:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
            }
        }
        break;

    default:
        memcpy(dst_p, src_p, dst->size < src->size ? dst->size : src->size);
    }
}

EL_ATTR_WEAK void rgb565_to_gray(const el_img_t* src, el_img_t* dst) {
    uint16_t sw = src->width;
    uint16_t sh = src->height;
    uint16_t dw = dst->width;
    uint16_t dh = dst->height;

    uint32_t beta_w = (sw << 16) / dw;
    uint32_t beta_h = (sh << 16) / dh;

    uint32_t i_mul_bh_sw = 0;
    uint32_t i_mul_dw    = 0;

    uint32_t init_index = 0;
    uint32_t index      = 0;

    const uint8_t* src_p = src->data;
    uint8_t*       dst_p = dst->data;

    b16_t b16{};

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    switch (dst->rotate) {
    case EL_PIXEL_ROTATE_90:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) | ((b16.b8_16 & 0xE0) >> 5)];

                dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
            }
        }
        break;

    case EL_PIXEL_ROTATE_180:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) | ((b16.b8_16 & 0xE0) >> 5)];

                dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
            }
        }
        break;

    case EL_PIXEL_ROTATE_270:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) | ((b16.b8_16 & 0xE0) >> 5)];

                dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
            }
        }
        break;

    default:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;
            i_mul_dw    = i * dw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = j + i_mul_dw;

                b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) | ((b16.b8_16 & 0xE0) >> 5)];

                dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
            }
        }
    }
}

EL_ATTR_WEAK void gray_to_rgb888(const el_img_t* src, el_img_t* dst) {
    uint16_t sw = src->width;
    uint16_t sh = src->height;
    uint16_t dw = dst->width;
    uint16_t dh = dst->height;

    uint32_t beta_w = (sw << 16) / dw;
    uint32_t beta_h = (sh << 16) / dh;

    uint32_t i_mul_bh_sw = 0;
    uint32_t i_mul_dw    = 0;

    uint32_t init_index = 0;
    uint32_t index      = 0;

    const uint8_t* src_p = src->data;
    uint8_t*       dst_p = dst->data;

    uint8_t c = 0;

    switch (dst->rotate) {
    case EL_PIXEL_ROTATE_90:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                c = src_p[init_index];

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) = b24_t{.b0_8 = c, .b8_16 = c, .b16_24 = c};
            }
        }
        break;

    case EL_PIXEL_ROTATE_180:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                c = src_p[init_index];

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) = b24_t{.b0_8 = c, .b8_16 = c, .b16_24 = c};
            }
        }
        break;

    case EL_PIXEL_ROTATE_270:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                c = src_p[init_index];

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) = b24_t{.b0_8 = c, .b8_16 = c, .b16_24 = c};
            }
        }
        break;

    default:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;
            i_mul_dw    = i * dw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = j + i_mul_dw;

                c = src_p[init_index];

                *reinterpret_cast<b24_t*>(dst_p + (index * 3)) = b24_t{.b0_8 = c, .b8_16 = c, .b16_24 = c};
            }
        }
    }
}

EL_ATTR_WEAK void gray_to_rgb565(const el_img_t* src, el_img_t* dst) {
    uint16_t sw = src->width;
    uint16_t sh = src->height;
    uint16_t dw = dst->width;
    uint16_t dh = dst->height;

    uint32_t beta_w = (sw << 16) / dw;
    uint32_t beta_h = (sh << 16) / dh;

    uint32_t i_mul_bh_sw = 0;
    uint32_t i_mul_dw    = 0;

    uint32_t init_index = 0;
    uint32_t index      = 0;

    const uint8_t* src_p = src->data;
    uint8_t*       dst_p = dst->data;

    uint8_t c = 0;

    switch (dst->rotate) {
    case EL_PIXEL_ROTATE_90:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                c = src_p[init_index];

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  b16_t{.b0_8  = static_cast<uint8_t>((c & 0xF8) | (c >> 5)),
                        .b8_16 = static_cast<uint8_t>(((c << 3) & 0xE0) | (c >> 3))};
            }
        }
        break;

    case EL_PIXEL_ROTATE_180:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                c = src_p[init_index];

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  b16_t{.b0_8  = static_cast<uint8_t>((c & 0xF8) | (c >> 5)),
                        .b8_16 = static_cast<uint8_t>(((c << 3) & 0xE0) | (c >> 3))};
            }
        }
        break;

    case EL_PIXEL_ROTATE_270:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                c = src_p[init_index];

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  b16_t{.b0_8  = static_cast<uint8_t>((c & 0xF8) | (c >> 5)),
                        .b8_16 = static_cast<uint8_t>(((c << 3) & 0xE0) | (c >> 3))};
            }
        }
        break;

    default:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;
            i_mul_dw    = i * dw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = j + i_mul_dw;

                c = src_p[init_index];

                *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                  b16_t{.b0_8  = static_cast<uint8_t>((c & 0xF8) | (c >> 5)),
                        .b8_16 = static_cast<uint8_t>(((c << 3) & 0xE0) | (c >> 3))};
            }
        }
    }
}

EL_ATTR_WEAK void gray_to_gray(const el_img_t* src, el_img_t* dst) {
    uint16_t sw = src->width;
    uint16_t sh = src->height;
    uint16_t dw = dst->width;
    uint16_t dh = dst->height;

    uint32_t beta_w = (sw << 16) / dw;
    uint32_t beta_h = (sh << 16) / dh;

    uint32_t i_mul_bh_sw = 0;

    uint32_t init_index = 0;
    uint32_t index      = 0;

    const uint8_t* src_p = src->data;
    uint8_t*       dst_p = dst->data;

    switch (dst->rotate) {
    case EL_PIXEL_ROTATE_90:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                dst_p[index] = src_p[init_index];
            }
        }
        break;

    case EL_PIXEL_ROTATE_180:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                dst_p[index] = src_p[init_index];
            }
        }
        break;

    case EL_PIXEL_ROTATE_270:
        for (uint16_t i = 0; i < dh; ++i) {
            i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

            for (uint16_t j = 0; j < dw; ++j) {
                init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                dst_p[index] = src_p[init_index];
            }
        }
        break;

    default:
        memcpy(dst_p, src_p, dst->size < src->size ? dst->size : src->size);
    }
}

// Note: Current downscaling algorithm implementation is INTER_NEARST
EL_ATTR_WEAK void rgb_to_rgb(const el_img_t* src, el_img_t* dst) {
    if (src->format == EL_PIXEL_FORMAT_RGB888) {
        if (dst->format == EL_PIXEL_FORMAT_RGB888)
            rgb888_to_rgb888(src, dst);
        else if (dst->format == EL_PIXEL_FORMAT_RGB565)
            rgb888_to_rgb565(src, dst);
        else if (dst->format == EL_PIXEL_FORMAT_GRAYSCALE)
            rgb888_to_gray(src, dst);
    }

    else if (src->format == EL_PIXEL_FORMAT_RGB565) {
        if (dst->format == EL_PIXEL_FORMAT_RGB888)
            rgb565_to_rgb888(src, dst);
        else if (dst->format == EL_PIXEL_FORMAT_RGB565)
            rgb565_to_rgb565(src, dst);
        else if (dst->format == EL_PIXEL_FORMAT_GRAYSCALE)
            rgb565_to_gray(src, dst);
    }

    else if (src->format == EL_PIXEL_FORMAT_GRAYSCALE) {
        if (dst->format == EL_PIXEL_FORMAT_RGB888)
            gray_to_rgb888(src, dst);
        else if (dst->format == EL_PIXEL_FORMAT_RGB565)
            gray_to_rgb565(src, dst);
        else if (dst->format == EL_PIXEL_FORMAT_GRAYSCALE)
            gray_to_gray(src, dst);
    }
}

#ifdef CONFIG_EL_LIB_JPEGENC

EL_ATTR_WEAK el_err_code_t rgb_to_jpeg(const el_img_t* src, el_img_t* dst) {
    static JPEG   jpg;
    JPEGENCODE    jpe;
    int           rc            = 0;
    el_err_code_t err           = EL_OK;
    int           iMCUCount     = 0;
    int           pitch         = 0;
    int           bytesPerPixel = 0;
    int           pixelFormat   = 0;
    EL_ASSERT(src->format == EL_PIXEL_FORMAT_GRAYSCALE || src->format == EL_PIXEL_FORMAT_RGB565 ||
              src->format == EL_PIXEL_FORMAT_RGB888);
    if (src->format == EL_PIXEL_FORMAT_GRAYSCALE) {
        bytesPerPixel = 1;
        pixelFormat   = JPEG_PIXEL_GRAYSCALE;
    } else if (src->format == EL_PIXEL_FORMAT_RGB565) {
        bytesPerPixel = 2;
        pixelFormat   = JPEG_PIXEL_RGB565;
    } else if (src->format == EL_PIXEL_FORMAT_RGB888) {
        bytesPerPixel = 3;
        pixelFormat   = JPEG_PIXEL_RGB888;
    }
    pitch = src->width * bytesPerPixel;
    rc    = jpg.open(dst->data, dst->size);
    if (rc != JPEG_SUCCESS) {
        err = EL_EIO;
        goto exit;
    }
    rc = jpg.encodeBegin(&jpe, src->width, src->height, pixelFormat, JPEG_SUBSAMPLE_444, JPEG_Q_LOW);
    if (rc != JPEG_SUCCESS) {
        err = EL_EIO;
        goto exit;
    }
    iMCUCount = ((src->width + jpe.cx - 1) / jpe.cx) * ((src->height + jpe.cy - 1) / jpe.cy);
    for (int i = 0; i < iMCUCount && rc == JPEG_SUCCESS; i++) {
        rc = jpg.addMCU(&jpe, &src->data[jpe.x * bytesPerPixel + jpe.y * src->width * bytesPerPixel], pitch);
    }
    if (rc != JPEG_SUCCESS) {
        err = EL_EIO;
        goto exit;
    }
    dst->size = jpg.close();

exit:
    return err;
}

#endif

// TODO: need to be optimized
EL_ATTR_WEAK el_err_code_t el_img_convert(const el_img_t* src, el_img_t* dst) {
    if (!src || !src->data) [[unlikely]]
        return EL_EINVAL;

    if (!dst || !dst->data) [[unlikely]]
        return EL_EINVAL;

    if (src->format == EL_PIXEL_FORMAT_RGB565 || src->format == EL_PIXEL_FORMAT_RGB888 ||
        src->format == EL_PIXEL_FORMAT_GRAYSCALE) {
        if (dst->format == EL_PIXEL_FORMAT_JPEG) {
            return rgb_to_jpeg(src, dst);
        }

        if (dst->format == EL_PIXEL_FORMAT_RGB565 || dst->format == EL_PIXEL_FORMAT_RGB888 ||
            dst->format == EL_PIXEL_FORMAT_GRAYSCALE) {
            rgb_to_rgb(src, dst);
            return EL_OK;
        }
    }

    if (src->format == EL_PIXEL_FORMAT_YUV422) {
        if (dst->format == EL_PIXEL_FORMAT_RGB565 || dst->format == EL_PIXEL_FORMAT_RGB888 ||
            dst->format == EL_PIXEL_FORMAT_GRAYSCALE) {
            yuv422p_to_rgb(src, dst);
            return EL_OK;
        }
    }

    return EL_ENOTSUP;
}

// TODO: need to be optimized
EL_ATTR_WEAK void el_draw_point(el_img_t* img, int16_t x, int16_t y, uint32_t color) {
    size_t   index = 0;
    uint8_t* data  = img->data;

    switch (img->format) {
    case EL_PIXEL_FORMAT_GRAYSCALE:
        index = x + y * img->width;
        if (index >= img->size) return;
        data[index] = color;
        break;

    case EL_PIXEL_FORMAT_RGB565:
        index = (x + y * img->width) * 2;
        if (index >= img->size) return;
        data[index]     = color & 0xFF;
        data[index + 1] = (color >> 8) & 0xFF;
        break;

    case EL_PIXEL_FORMAT_RGB888:
        index = (x + y * img->width) * 3;
        if (index >= img->size) return;
        data[index]     = color & 0xFF;
        data[index + 1] = (color >> 8) & 0xFF;
        data[index + 2] = (color >> 16) & 0xFF;
        break;

    default:
        return;
    }
}

EL_ATTR_WEAK void el_fill_rect(el_img_t* img, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color) {
    if ((w >> 15) || (h >> 15)) [[unlikely]]
        return;

    el_pixel_format_t format = img->format;
    uint16_t          iw     = img->width;
    uint16_t          ih     = img->height;

    x = x >= iw ? iw : x & ~(x >> 15);
    y = y >= ih ? ih : y & ~(y >> 15);
    w = x + w >= iw ? iw - x : w;
    h = y + h >= ih ? ih - y : h;

    int32_t  line_step = 0;
    uint8_t* data      = nullptr;
    uint8_t  c0        = (color >> 16) & 0xFF;
    uint8_t  c1        = (color >> 8) & 0xFF;
    uint8_t  c2        = color;

    switch (format) {
    case EL_PIXEL_FORMAT_GRAYSCALE:
        line_step = iw - w;
        data      = img->data + x + (y * iw);

        for (int i = 0; i < h; ++i, data += line_step) {
            memset(data, c2, w);
        }
        break;

    case EL_PIXEL_FORMAT_RGB565:
        line_step = (iw - w) * 2;
        data      = img->data + ((x + (y * iw)) * 2);
        for (int i = 0; i < h; ++i, data += line_step) {
            for (int j = 0; j < w; ++j, data += 2) {
                data[0] = c1;
                data[1] = c2;
            }
        }
        break;

    case EL_PIXEL_FORMAT_RGB888:
        line_step = (iw - w) * 3;
        data      = img->data + ((x + (y * iw)) * 3);
        for (int i = 0; i < h; ++i, data += line_step) {
            for (int j = 0; j < w; ++j, data += 3) {
                data[0] = c0;
                data[1] = c1;
                data[2] = c2;
            }
        }
        break;

    default:
        return;
    }
}

// TODO: need to be optimized
EL_ATTR_WEAK void el_draw_h_line(el_img_t* img, int16_t x0, int16_t x1, int16_t y, uint32_t color) {
    el_fill_rect(img, x0, y, x1 - x0, 1, color);
}

// TODO: need to be optimized
EL_ATTR_WEAK void el_draw_v_line(el_img_t* img, int16_t x, int16_t y0, int16_t y1, uint32_t color) {
    el_fill_rect(img, x, y0, 1, y1 - y0, color);
}

// TODO: need to be optimized
EL_ATTR_WEAK void el_draw_rect(
  el_img_t* img, int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color, uint8_t thickness) {
    int16_t x_add_i = 0;
    int16_t x_sub_i = 0;
    int16_t y_add_i = 0;
    int16_t y_sub_i = 0;
    for (uint8_t i = 0; i < thickness; ++i) {
        x_add_i = x + i;
        x_sub_i = x - i;
        y_add_i = y + i;
        y_sub_i = y - i;
        el_draw_h_line(img, x_add_i, (x_sub_i + w), y_add_i, color);
        el_draw_h_line(img, x_add_i, (x_sub_i + w) + 1, (y_sub_i + h), color);
        el_draw_v_line(img, x_add_i, y_add_i, (y_sub_i + h), color);
        el_draw_v_line(img, (x_sub_i + w), y_add_i, (y_sub_i + h) + 1, color);
    }
}

}  // namespace edgelab
