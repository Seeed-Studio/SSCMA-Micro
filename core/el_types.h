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

#ifndef _EL_TYPES_H_
#define _EL_TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "el_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EL_OK      = 0,  // success
    EL_AGAIN   = 1,  // try again
    EL_ELOG    = 2,  // logic error
    EL_ETIMOUT = 3,  // timeout
    EL_EIO     = 4,  // IO error
    EL_EINVAL  = 5,  // invalid argument
    EL_ENOMEM  = 6,  // out of memory
    EL_EBUSY   = 7,  // busy
    EL_ENOTSUP = 8,  // not supported
    EL_EPERM   = 9,  // operation not permitted
} el_err_code_t;

typedef struct EL_ATTR_PACKED el_shape_t {
    size_t size;
    int*   dims;
} el_shape_t;

typedef struct EL_ATTR_PACKED el_quant_param_t {
    float   scale;
    int32_t zero_point;
} el_quant_param_t;

typedef struct EL_ATTR_PACKED el_memory_pool_t {
    void*  pool;
    size_t size;
} el_memory_pool_t;

typedef struct EL_ATTR_PACKED el_box_t {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint8_t  score;
    uint8_t  target;
} el_box_t;

typedef struct EL_ATTR_PACKED el_point_t {
    uint16_t x;
    uint16_t y;
    uint8_t  score;
    uint8_t  target;
} el_point_t;

typedef struct EL_ATTR_PACKED el_class_t {
    uint16_t score;
    uint16_t target;
} el_class_t;

typedef struct EL_ATTR_PACKED el_tensor_t {
    size_t index;
    void*  data;
    size_t size;
} el_tensor_t;

typedef struct EL_ATTR_PACKED el_input_t {
    el_tensor_t* tensor;
    size_t       length;
} el_input_t;

typedef enum {
    EL_PIXEL_FORMAT_RGB888 = 0,
    EL_PIXEL_FORMAT_RGB565,
    EL_PIXEL_FORMAT_YUV422,
    EL_PIXEL_FORMAT_GRAYSCALE,
    EL_PIXEL_FORMAT_JPEG,
    EL_PIXEL_FORMAT_UNKNOWN,
} el_pixel_format_t;

typedef enum el_pixel_rotate_t {
    EL_PIXEL_ROTATE_0 = 0,
    EL_PIXEL_ROTATE_90,
    EL_PIXEL_ROTATE_180,
    EL_PIXEL_ROTATE_270,
    EL_PIXEL_ROTATE_UNKNOWN,
} el_pixel_rotate_t;

typedef struct EL_ATTR_PACKED el_img_t {
    uint8_t*          data;
    size_t            size;
    uint16_t          width;
    uint16_t          height;
    el_pixel_format_t format;
    el_pixel_rotate_t rotate;
} el_img_t;

typedef struct EL_ATTR_PACKED el_res_t {
    uint32_t width;
    uint32_t height;
} el_res_t;

typedef enum { EL_SENSOR_TYPE_CAM = 1u } el_sensor_type_t;

typedef enum { EL_SENSOR_STA_REG = 0u, EL_SENSOR_STA_AVAIL = 1u, EL_SENSOR_STA_LOCKED = 2u } el_sensor_state_t;

typedef struct EL_ATTR_PACKED el_sensor_info_t {
    uint8_t           id;
    el_sensor_type_t  type;
    el_sensor_state_t state;
} el_sensor_info_t;

typedef enum {
    EL_MODEL_FMT_UNDEFINED     = 0u,
    EL_MODEL_FMT_PACKED_TFLITE = 1u,
    EL_MODEL_FMT_PLAIN_TFLITE  = 2u
} el_model_format_t;

typedef int el_model_format_v;

/**
 * @brief Algorithm Types
 */
typedef enum {
    EL_ALGO_TYPE_UNDEFINED = 0u,
    EL_ALGO_TYPE_FOMO      = 1u,
    EL_ALGO_TYPE_PFLD      = 2u,
    EL_ALGO_TYPE_YOLO      = 3u,
    EL_ALGO_TYPE_IMCLS     = 4u
} el_algorithm_type_t;

/**
 * @brief Algorithm Catagories
 */
typedef enum {
    EL_ALGO_CAT_UNDEFINED = 0u,
    EL_ALGO_CAT_DET       = 1u,
    EL_ALGO_CAT_POSE      = 2u,
    EL_ALGO_CAT_CLS       = 3u,
} el_algorithm_cat_t;

/**
 * @brief Model Header Specification
 * @details
 *      [ 24 bits magic code | 4 bits id | 4 bits type | 24 bits size (unsigned) | 8 bits unused padding ]
 *      big-endian in file
 */
typedef union EL_ATTR_PACKED el_model_header_t {
    unsigned char b1[8];
    uint32_t      b4[2];
} el_model_header_t;

/**
 * @brief Mdoel Info Specification
 * @details
 *      valid id range [1, 15]:
 *          0 -> undefined
 *          each model should have a unique id
 *      valid type range [1, 15]:
 *          0 -> undefined
 *          1 -> FOMO
 *          2 -> PFLD
 *          3 -> YOLO
 */
typedef struct EL_ATTR_PACKED el_model_info_t {
    uint8_t             id;
    el_algorithm_type_t type;
    uint32_t            addr_flash;
    uint32_t            size;
    const uint8_t*      addr_memory;
} el_model_info_t;

typedef uint8_t el_model_id_t;

#ifdef __cplusplus
}
#endif

#endif
