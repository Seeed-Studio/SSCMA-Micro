#include "ma_cv.h"

#if MA_USE_LIB_JPEGENC
#include "JPEGENC.h"
#endif

namespace ma::cv {

static const char* TAG = "ma::cv";

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

typedef struct MA_ATTR_PACKED b16_t {
    uint8_t b0_8;
    uint8_t b8_16;
} b16_t;

typedef struct MA_ATTR_PACKED b24_t {
    uint8_t b0_8;
    uint8_t b8_16;
    uint8_t b16_24;
} b24_t;

// TODO: need to be optimized
MA_ATTR_WEAK void yuv422p_to_rgb(const ma_img_t* src, ma_img_t* dst) {
    int32_t y;
    int32_t cr;
    int32_t cb;
    int32_t r, g, b;
    uint32_t init_index, cbcr_index, index;
    uint32_t u_chunk = src->width * src->height;
    uint32_t v_chunk = src->width * src->height + src->width * src->height / 2;
    float beta_h = (float)src->height / dst->height, beta_w = (float)src->width / dst->width;

    MA_ASSERT(src->format == MA_PIXEL_FORMAT_YUV422);

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
                case MA_PIXEL_ROTATE_90:
                    index = (index % dst->width) * (dst->height) +
                        (dst->height - 1 - index / dst->width);
                    break;
                case MA_PIXEL_ROTATE_180:
                    index = (dst->width - 1 - index % dst->width) +
                        (dst->height - 1 - index / dst->width) * (dst->width);
                    break;
                case MA_PIXEL_ROTATE_270:
                    index =
                        (dst->width - 1 - index % dst->width) * (dst->height) + index / dst->width;
                    break;
                default:
                    break;
            }
            if (dst->format == MA_PIXEL_FORMAT_GRAYSCALE) {
                uint8_t gray     = (r * 299 + g * 587 + b * 114) / 1000;
                dst->data[index] = (uint8_t)MA_CLIP(gray, 0, 255);
            } else if (dst->format == MA_PIXEL_FORMAT_RGB565) {
                dst->data[index * 2 + 0] = (r & 0xF8) | (g >> 5);
                dst->data[index * 2 + 1] = ((g << 3) & 0xE0) | (b >> 3);
            } else if (dst->format == MA_PIXEL_FORMAT_RGB888) {
                dst->data[index * 3 + 0] = (uint8_t)MA_CLIP(r, 0, 255);
                dst->data[index * 3 + 1] = (uint8_t)MA_CLIP(g, 0, 255);
                dst->data[index * 3 + 2] = (uint8_t)MA_CLIP(b, 0, 255);
            }
        }
    }
}

MA_ATTR_WEAK void rgb888_to_rgb888(const ma_img_t* src, ma_img_t* dst) {
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
    uint8_t* dst_p       = dst->data;


    switch (dst->rotate) {
        case MA_PIXEL_ROTATE_90:
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

        case MA_PIXEL_ROTATE_180:
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

        case MA_PIXEL_ROTATE_270:
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
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = j + i * dw;

                    *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                        *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                }
            }
            break;
    }
}

MA_ATTR_WEAK void rgb888_to_rgb888_planar(const ma_img_t* src, ma_img_t* dst) {
    uint16_t sw = src->width;
    uint16_t sh = src->height;
    uint16_t dw = dst->width;
    uint16_t dh = dst->height;

    uint32_t beta_w = (sw << 16) / dw;
    uint32_t beta_h = (sh << 16) / dh;

    uint32_t i_mul_bh_sw = 0;

    uint32_t init_index  = 0;
    uint32_t index       = 0;
    uint32_t planar_size = dw * dh;

    const uint8_t* src_p = src->data;
    uint8_t* dst_p       = dst->data;


    switch (dst->rotate) {
        case MA_PIXEL_ROTATE_90:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index                     = (((j * beta_w) >> 16) + i_mul_bh_sw) * 3;
                    index                          = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));
                    dst_p[index]                   = src_p[init_index + 0];
                    dst_p[index + planar_size]     = src_p[init_index + 1];
                    dst_p[index + planar_size * 2] = src_p[init_index + 2];
                }
            }
            break;

        case MA_PIXEL_ROTATE_180:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;
                for (uint16_t j = 0; j < dw; ++j) {
                    init_index   = (((j * beta_w) >> 16) + i_mul_bh_sw) * 3;
                    index        = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;
                    dst_p[index] = src_p[init_index + 0];
                    dst_p[index + planar_size]     = src_p[init_index + 1];
                    dst_p[index + planar_size * 2] = src_p[init_index + 2];
                }
            }
            break;

        case MA_PIXEL_ROTATE_270:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index                     = (((j * beta_w) >> 16) + i_mul_bh_sw) * 3;
                    index                          = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;
                    dst_p[index]                   = src_p[init_index + 0];
                    dst_p[index + planar_size]     = src_p[init_index + 1];
                    dst_p[index + planar_size * 2] = src_p[init_index + 2];
                }
            }
            break;

        default:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;
                for (uint16_t j = 0; j < dw; ++j) {
                    init_index                     = (((j * beta_w) >> 16) + i_mul_bh_sw) * 3;
                    index                          = (j + i * dw);
                    dst_p[index]                   = src_p[init_index + 0];
                    dst_p[index + planar_size]     = src_p[init_index + 1];
                    dst_p[index + planar_size * 2] = src_p[init_index + 2];
                }
            }
            break;
    }
}

MA_ATTR_WEAK void rgb888_to_rgb565(const ma_img_t* src, ma_img_t* dst) {
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
    uint8_t* dst_p       = dst->data;

    b24_t b24{};
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    switch (dst->rotate) {
        case MA_PIXEL_ROTATE_90:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                    b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) |
                                                        ((b24.b8_16 & 0xE0) >> 5)];

                    *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                        b16_t{.b0_8  = static_cast<uint8_t>((r & 0xF8) | (g >> 5)),
                              .b8_16 = static_cast<uint8_t>(((g << 3) & 0xE0) | (b >> 3))};
                }
            }
            break;

        case MA_PIXEL_ROTATE_180:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                    b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) |
                                                        ((b24.b8_16 & 0xE0) >> 5)];

                    *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                        b16_t{.b0_8  = static_cast<uint8_t>((r & 0xF8) | (g >> 5)),
                              .b8_16 = static_cast<uint8_t>(((g << 3) & 0xE0) | (b >> 3))};
                }
            }
            break;

        case MA_PIXEL_ROTATE_270:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                    b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) |
                                                        ((b24.b8_16 & 0xE0) >> 5)];

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
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) |
                                                        ((b24.b8_16 & 0xE0) >> 5)];

                    *reinterpret_cast<b16_t*>(dst_p + (index << 1)) =
                        b16_t{.b0_8  = static_cast<uint8_t>((r & 0xF8) | (g >> 5)),
                              .b8_16 = static_cast<uint8_t>(((g << 3) & 0xE0) | (b >> 3))};
                }
            }
    }
}

MA_ATTR_WEAK void rgb888_to_gray(const ma_img_t* src, ma_img_t* dst) {
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
    uint8_t* dst_p       = dst->data;

    b24_t b24{};

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    switch (dst->rotate) {
        case MA_PIXEL_ROTATE_90:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                    b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) |
                                                        ((b24.b8_16 & 0xE0) >> 5)];

                    dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
                }
            }
            break;

        case MA_PIXEL_ROTATE_180:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                    b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) |
                                                        ((b24.b8_16 & 0xE0) >> 5)];

                    dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
                }
            }
            break;

        case MA_PIXEL_ROTATE_270:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                    b24 = *reinterpret_cast<const b24_t*>(src_p + (init_index * 3));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b24.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b24.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) |
                                                        ((b24.b8_16 & 0xE0) >> 5)];

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
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b24.b0_8 & 0x07) << 3) |
                                                        ((b24.b8_16 & 0xE0) >> 5)];

                    dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
                }
            }
    }
}

MA_ATTR_WEAK void rgb565_to_rgb888(const ma_img_t* src, ma_img_t* dst) {
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
    uint8_t* dst_p       = dst->data;

    b16_t b16{};

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    switch (dst->rotate) {
        case MA_PIXEL_ROTATE_90:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                    b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) |
                                                        ((b16.b8_16 & 0xE0) >> 5)];

                    *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                        b24_t{.b0_8 = r, .b8_16 = g, .b16_24 = b};
                }
            }
            break;

        case MA_PIXEL_ROTATE_180:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                    b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) |
                                                        ((b16.b8_16 & 0xE0) >> 5)];

                    *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                        b24_t{.b0_8 = r, .b8_16 = g, .b16_24 = b};
                }
            }
            break;

        case MA_PIXEL_ROTATE_270:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                    b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) |
                                                        ((b16.b8_16 & 0xE0) >> 5)];

                    *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                        b24_t{.b0_8 = r, .b8_16 = g, .b16_24 = b};
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
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) |
                                                        ((b16.b8_16 & 0xE0) >> 5)];

                    *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                        b24_t{.b0_8 = r, .b8_16 = g, .b16_24 = b};
                }
            }
    }
}

MA_ATTR_WEAK void rgb565_to_rgb565(const ma_img_t* src, ma_img_t* dst) {
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
    uint8_t* dst_p       = dst->data;

    switch (dst->rotate) {
        case MA_PIXEL_ROTATE_90:
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

        case MA_PIXEL_ROTATE_180:
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

        case MA_PIXEL_ROTATE_270:
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

MA_ATTR_WEAK void rgb565_to_gray(const ma_img_t* src, ma_img_t* dst) {
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
    uint8_t* dst_p       = dst->data;

    b16_t b16{};

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    switch (dst->rotate) {
        case MA_PIXEL_ROTATE_90:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                    b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) |
                                                        ((b16.b8_16 & 0xE0) >> 5)];

                    dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
                }
            }
            break;

        case MA_PIXEL_ROTATE_180:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                    b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) |
                                                        ((b16.b8_16 & 0xE0) >> 5)];

                    dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
                }
            }
            break;

        case MA_PIXEL_ROTATE_270:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                    b16 = *reinterpret_cast<const b16_t*>(src_p + (init_index << 1));
                    r   = RGB565_TO_RGB888_LOOKUP_TABLE_5[(b16.b0_8 & 0xF8) >> 3];
                    b   = RGB565_TO_RGB888_LOOKUP_TABLE_5[b16.b8_16 & 0x1F];
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) |
                                                        ((b16.b8_16 & 0xE0) >> 5)];

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
                    g   = RGB565_TO_RGB888_LOOKUP_TABLE_6[((b16.b0_8 & 0x07) << 3) |
                                                        ((b16.b8_16 & 0xE0) >> 5)];

                    dst_p[index] = (r * 299 + g * 587 + b * 114) / 1000;
                }
            }
    }
}

MA_ATTR_WEAK void gray_to_rgb888(const ma_img_t* src, ma_img_t* dst) {
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
    uint8_t* dst_p       = dst->data;

    uint8_t c = 0;

    switch (dst->rotate) {
        case MA_PIXEL_ROTATE_90:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                    c = src_p[init_index];

                    *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                        b24_t{.b0_8 = c, .b8_16 = c, .b16_24 = c};
                }
            }
            break;

        case MA_PIXEL_ROTATE_180:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                    c = src_p[init_index];

                    *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                        b24_t{.b0_8 = c, .b8_16 = c, .b16_24 = c};
                }
            }
            break;

        case MA_PIXEL_ROTATE_270:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) * dh + (j / dw) + i;

                    c = src_p[init_index];

                    *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                        b24_t{.b0_8 = c, .b8_16 = c, .b16_24 = c};
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

                    *reinterpret_cast<b24_t*>(dst_p + (index * 3)) =
                        b24_t{.b0_8 = c, .b8_16 = c, .b16_24 = c};
                }
            }
    }
}

MA_ATTR_WEAK void gray_to_rgb565(const ma_img_t* src, ma_img_t* dst) {
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
    uint8_t* dst_p       = dst->data;

    uint8_t c = 0;

    switch (dst->rotate) {
        case MA_PIXEL_ROTATE_90:
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

        case MA_PIXEL_ROTATE_180:
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

        case MA_PIXEL_ROTATE_270:
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

MA_ATTR_WEAK void gray_to_gray(const ma_img_t* src, ma_img_t* dst) {
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
    uint8_t* dst_p       = dst->data;

    switch (dst->rotate) {
        case MA_PIXEL_ROTATE_90:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = (j % dw) * dh + ((dh - 1) - ((j / dw) + i));

                    dst_p[index] = src_p[init_index];
                }
            }
            break;

        case MA_PIXEL_ROTATE_180:
            for (uint16_t i = 0; i < dh; ++i) {
                i_mul_bh_sw = ((i * beta_h) >> 16) * sw;

                for (uint16_t j = 0; j < dw; ++j) {
                    init_index = ((j * beta_w) >> 16) + i_mul_bh_sw;
                    index      = ((dw - 1) - (j % dw)) + ((dh - 1) - ((j / dw) + i)) * dw;

                    dst_p[index] = src_p[init_index];
                }
            }
            break;

        case MA_PIXEL_ROTATE_270:
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
MA_ATTR_WEAK ma_err_t rgb_to_rgb(const ma_img_t* src, ma_img_t* dst) {
    if (src->format == MA_PIXEL_FORMAT_RGB888) {
        if (dst->format == MA_PIXEL_FORMAT_RGB888)
            rgb888_to_rgb888(src, dst);
        else if (dst->format == MA_PIXEL_FORMAT_RGB888_PLANAR)
            rgb888_to_rgb888_planar(src, dst);
        else if (dst->format == MA_PIXEL_FORMAT_RGB565)
            rgb888_to_rgb565(src, dst);
        else if (dst->format == MA_PIXEL_FORMAT_GRAYSCALE)
            rgb888_to_gray(src, dst);
        else
            return MA_ENOTSUP;
    }

    else if (src->format == MA_PIXEL_FORMAT_RGB565) {
        if (dst->format == MA_PIXEL_FORMAT_RGB888)
            rgb565_to_rgb888(src, dst);
        else if (dst->format == MA_PIXEL_FORMAT_RGB565)
            rgb565_to_rgb565(src, dst);
        else if (dst->format == MA_PIXEL_FORMAT_GRAYSCALE)
            rgb565_to_gray(src, dst);
        else
            return MA_ENOTSUP;
    }

    else if (src->format == MA_PIXEL_FORMAT_GRAYSCALE) {
        if (dst->format == MA_PIXEL_FORMAT_RGB888)
            gray_to_rgb888(src, dst);
        else if (dst->format == MA_PIXEL_FORMAT_RGB565)
            gray_to_rgb565(src, dst);
        else if (dst->format == MA_PIXEL_FORMAT_GRAYSCALE)
            gray_to_gray(src, dst);
        else
            return MA_ENOTSUP;
    }

    return MA_OK;
}

#if MA_USE_LIB_JPEGENC

MA_ATTR_WEAK ma_err_t rgb_to_jpeg(const ma_img_t* src, ma_img_t* dst) {
    static JPEG jpg;
    JPEGENCODE jpe;
    int rc            = 0;
    ma_err_t err      = MA_OK;
    int iMCUCount     = 0;
    int pitch         = 0;
    int bytesPerPixel = 0;
    int pixelFormat   = 0;
    // MA_LOGD(TAG, "rgb_to_jpeg");
    MA_ASSERT(src->format == MA_PIXEL_FORMAT_GRAYSCALE || src->format == MA_PIXEL_FORMAT_RGB565 ||
              src->format == MA_PIXEL_FORMAT_RGB888);
    if (src->format == MA_PIXEL_FORMAT_GRAYSCALE) {
        bytesPerPixel = 1;
        pixelFormat   = JPEG_PIXEL_GRAYSCALE;
    } else if (src->format == MA_PIXEL_FORMAT_RGB565) {
        bytesPerPixel = 2;
        pixelFormat   = JPEG_PIXEL_RGB565;
    } else if (src->format == MA_PIXEL_FORMAT_RGB888) {
        bytesPerPixel = 3;
        pixelFormat   = JPEG_PIXEL_RGB888;
    }
    pitch = src->width * bytesPerPixel;
    rc    = jpg.open(dst->data, dst->size);
    if (rc != JPEG_SUCCESS) {
        err = MA_EIO;
        goto exit;
    }
    rc =
        jpg.encodeBegin(&jpe, src->width, src->height, pixelFormat, JPEG_SUBSAMPLE_444, JPEG_Q_LOW);
    if (rc != JPEG_SUCCESS) {
        err = MA_EIO;
        goto exit;
    }
    iMCUCount = ((src->width + jpe.cx - 1) / jpe.cx) * ((src->height + jpe.cy - 1) / jpe.cy);
    for (int i = 0; i < iMCUCount && rc == JPEG_SUCCESS; i++) {
        rc = jpg.addMCU(
            &jpe, &src->data[jpe.x * bytesPerPixel + jpe.y * src->width * bytesPerPixel], pitch);
    }
    if (rc != JPEG_SUCCESS) {
        err = MA_EIO;
        goto exit;
    }
    dst->size = jpg.close();

exit:
    return err;
}

#endif

// TODO: need to be optimized
MA_ATTR_WEAK ma_err_t convert(const ma_img_t* src, ma_img_t* dst) {
    if (!src || !src->data) [[unlikely]]
        return MA_EINVAL;

    if (!dst || !dst->data) [[unlikely]]
        return MA_EINVAL;

    if (src->format == dst->format && src->width == dst->width &&
        src->height == dst->height && src->data != dst->data) {
        memcpy(dst->data, src->data, src->size);
        return MA_OK;
    }

    if (src->format == MA_PIXEL_FORMAT_RGB565 || src->format == MA_PIXEL_FORMAT_RGB888 ||
        src->format == MA_PIXEL_FORMAT_GRAYSCALE) {
#if MA_USE_LIB_JPEGENC
        if (dst->format == MA_PIXEL_FORMAT_JPEG) {
            return rgb_to_jpeg(src, dst);
        }
#endif

        if (dst->format == MA_PIXEL_FORMAT_RGB565 || dst->format == MA_PIXEL_FORMAT_RGB888 ||
            dst->format == MA_PIXEL_FORMAT_RGB888_PLANAR || dst->format == MA_PIXEL_FORMAT_GRAYSCALE) {
            return rgb_to_rgb(src, dst);
        }
    }

    if (src->format == MA_PIXEL_FORMAT_YUV422) {
        if (dst->format == MA_PIXEL_FORMAT_RGB565 || dst->format == MA_PIXEL_FORMAT_RGB888 ||
            dst->format == MA_PIXEL_FORMAT_GRAYSCALE) {
            yuv422p_to_rgb(src, dst);
            return MA_OK;
        }
    }

    return MA_ENOTSUP;
}


}  // namespace ma::cv