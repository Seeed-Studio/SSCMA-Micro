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

#ifndef _EL_CONFIG_H_
#define _EL_CONFIG_H_

#define CONFIG_EL_TARGET_WE1
#define CONFIG_EL_BOARD_GROVE_VISION_AI

#define CONFIG_EL_DEBUG 4

#define CONFIG_EL_MODEL
#define CONFIG_EL_MODEL_TFLITE_MAGIC   0x54464C33
#define CONFIG_EL_MODEL_HEADER_MAGIC   0x4C4854
#define CONFIG_EL_MODEL_PARTITION_NAME "models"

#define CONFIG_EL_LIB_FLASHDB
#define CONFIG_EL_STORAGE
#define CONFIG_EL_STORAGE_NAME                  "edgelab_db"
#define CONFIG_EL_STORAGE_PATH                  "kvdb0"
#define CONFIG_EL_STORAGE_PARTITION_NAME        "db"
#define CONFIG_EL_STORAGE_PARTITION_MOUNT_POINT "nor_flash0"
#define CONFIG_EL_STORAGE_PARTITION_FS_NAME_0   "kvdb0"
#define CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0   (64 * 1024)
#define CONFIG_EL_STORAGE_KEY_SIZE_MAX          (64)

#define CONFIG_EL_TFLITE                        1
#define CONFIG_EL_TFLITE_OP_PADV2               1
// #define CONFIG_EL_TFLITE_OP_TRANSPOSE           1
// #define CONFIG_EL_TFLITE_OP_ETHOS_U             1
// #define CONFIG_EL_TFLITE_OP_DEQUANTIZE          1
// #define CONFIG_EL_TFLITE_OP_QUANTIZE            1
// #define CONFIG_EL_TFLITE_OP_FULLY_CONNECTED     1
// #define CONFIG_EL_TFLITE_OP_GATHER              1

#define CONFIG_EL_HAS_FREERTOS_SUPPORT          1

#endif
