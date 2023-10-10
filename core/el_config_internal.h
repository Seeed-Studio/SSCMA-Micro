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

#ifndef _EL_CONFIG_INTERNAL_H_
#define _EL_CONFIG_INTERNAL_H_

#include "core/el_config.h"

/* debug config check */
#ifndef CONFIG_EL_DEBUG
    // Level:
    //      0: no debug
    //      1: print error
    //      2: print warning
    //      3: print info
    //      4: print verbose
    #define CONFIG_EL_DEBUG 3
    #define EL_DEBUG_COLOR
#endif

/* porting config check */
#ifndef CONFIG_EL_PORTING_POSIX
    #if defined(__unix__) || defined(__APPLE__)
        #define CONFIG_EL_PORTING_POSIX 1
    #else
        #define CONFIG_EL_PORTING_POSIX 0
    #endif
#endif

/* os related config check */
#ifndef CONFIG_EL_HAS_FREERTOS_SUPPORT
    #define CONFIG_EL_HAS_FREERTOS_SUPPORT 1
#endif

/* engine related config */
#ifndef CONFIG_EL_TFLITE
    #define CONFIG_EL_TFLITE
    #define CONFIG_EL_TFLITE_OP_CONV_2D
    #define CONFIG_EL_TFLITE_OP_RESHAPE
    #define CONFIG_EL_TFLITE_OP_SHAPE
    #define CONFIG_EL_TFLITE_OP_PACK
    #define CONFIG_EL_TFLITE_OP_PAD
    #define CONFIG_EL_TFLITE_OP_PADV2
    #define CONFIG_EL_TFLITE_OP_SUB
    #define CONFIG_EL_TFLITE_OP_ADD
    #define CONFIG_EL_TFLITE_OP_RELU
    #define CONFIG_EL_TFLITE_OP_MAX_POOL_2D
    #define CONFIG_EL_TFLITE_OP_SPLIT
    #define CONFIG_EL_TFLITE_OP_CONCATENATION
    #define CONFIG_EL_TFLITE_OP_FULLY_CONNECTED
    #define CONFIG_EL_TFLITE_OP_RESIZE_NEAREST_NEIGHBOR
    #define CONFIG_EL_TFLITE_OP_QUANTIZE
    #define CONFIG_EL_TFLITE_OP_TRANSPOSE
    #define CONFIG_EL_TFLITE_OP_LOGISTIC
    #define CONFIG_EL_TFLITE_OP_MUL
    #define CONFIG_EL_TFLITE_OP_SPLIT_V
    #define CONFIG_EL_TFLITE_OP_STRIDED_SLICE
    #define CONFIG_EL_TFLITE_OP_MEAN
    #define CONFIG_EL_TFLITE_OP_SOFTMAX
    #define CONFIG_EL_TFLITE_OP_DEPTHWISE_CONV_2D
    #define CONFIG_EL_TFLITE_OP_LEAKY_RELU
#endif

/* model related config */
#ifndef CONFIG_EL_MODEL
    #define CONFIG_EL_MODEL_TFLITE_MAGIC   0x54464C33
    #define CONFIG_EL_MODEL_HEADER_MAGIC   0x4C4854
    #define CONFIG_EL_MODEL_PARTITION_NAME "models"
#endif

/* storage related config */
#ifndef CONFIG_EL_STORAGE
    #ifndef CONFIG_EL_LIB_FLASHDB
        #error "Storage depends on FlashDB."
    #endif
    #define CONFIG_EL_STORAGE_NAME                  "edgelab_db"
    #define CONFIG_EL_STORAGE_PATH                  "kvdb0"
    #define CONFIG_EL_STORAGE_PARTITION_NAME        "db"
    #define CONFIG_EL_STORAGE_PARTITION_MOUNT_POINT "nor_flash0"
    #define CONFIG_EL_STORAGE_PARTITION_FS_NAME_0   "kvdb0"
    #define CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0   (192 * 1024)
    #define CONFIG_EL_STORAGE_KEY_SIZE_MAX          (64)
#endif

#endif
