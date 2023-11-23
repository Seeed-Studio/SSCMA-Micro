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

#ifndef _EL_CONFIG_PORTING_H_
#define _EL_CONFIG_PORTING_H_

#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>
#include <timers.h>

#ifndef CONFIG_EL_TARGET_HIMAX
    #error "Please specify porting target"
#else
    #define VENDOR_PREFIX    "himax"
    #define VENDOR_CHIP_NAME "we1"
    #if defined(CONFIG_EL_BOARD_GRIVE_VISION_AI_WE1)
        #include "boards/grove_vision_ai_we1/board.h"
    #else
        #error "Please specify porting board"
    #endif
#endif

#define CONFIG_EL_DEBUG                3

#define CONFIG_EL_HAS_FREERTOS_SUPPORT 1
#define SSCMA_REPL_EXECUTOR_PRIO       2
#define CONFIG_SSCMA_TENSOR_ARENA_SIZE (600 * 1024)

#define CONFIG_EL_TFLITE
#define CONFIG_EL_TFLITE_OP_PADV2
#define CONFIG_EL_TFLITE_OP_TRANSPOSE
#define CONFIG_EL_TFLITE_OP_ETHOS_U
#define CONFIG_EL_TFLITE_OP_DEQUANTIZE
#define CONFIG_EL_TFLITE_OP_QUANTIZE
#define CONFIG_EL_TFLITE_OP_FULLY_CONNECTED
#define CONFIG_EL_TFLITE_OP_GATHER

#define CONFIG_EL_MODEL                         1
#define CONFIG_EL_MODEL_TFLITE_MAGIC            0x54464C33
#define CONFIG_EL_MODEL_HEADER_MAGIC            0x004C4854
#define CONFIG_EL_MODEL_PARTITION_NAME          "models"

#define CONFIG_EL_HAS_ACCELERATED_JPEG_CODEC    1

#define CONFIG_EL_LIB_FLASHDB                   1
#define CONFIG_EL_LIB_JPEGENC                   0

#define CONFIG_EL_STORAGE                       1
#define CONFIG_EL_STORAGE_NAME                  "edgelab_db"
#define CONFIG_EL_STORAGE_PATH                  "kvdb0"
#define CONFIG_EL_STORAGE_PARTITION_NAME        "db"
#define CONFIG_EL_STORAGE_PARTITION_MOUNT_POINT "nor_flash0"
#define CONFIG_EL_STORAGE_PARTITION_FS_NAME_0   "kvdb0"
#define CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0   (64 * 1024)
#define CONFIG_EL_STORAGE_KEY_SIZE_MAX          (64)

#if CONFIG_EL_LIB_FLASHDB
    #include "third_party/FlashDB/fal_def.h"

extern const struct fal_flash_dev _el_flash_db_nor_flash0;

    #define NOR_FLASH_DEV_NAME CONFIG_EL_STORAGE_PARTITION_MOUNT_POINT
    #define FAL_FLASH_DEV_TABLE \
        { &_el_flash_db_nor_flash0, }

    #define FAL_PART_HAS_TABLE_CFG
    #ifdef FAL_PART_HAS_TABLE_CFG
        #define FAL_PART_TABLE                          \
            {                                           \
                {FAL_PART_MAGIC_WORD,                   \
                 CONFIG_EL_STORAGE_PARTITION_FS_NAME_0, \
                 NOR_FLASH_DEV_NAME,                    \
                 0,                                     \
                 CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0, \
                 0},                                    \
            }
    #endif

    #define FDB_USING_KVDB
    #ifdef FDB_USING_KVDB
        #define FDB_KV_AUTO_UPDATE
    #endif

    #define FDB_USING_FAL_MODE
    #define FDB_WRITE_GRAN (32)
    #define FDB_BLOCK_SIZE (8 * 1024)

    #if CONFIG_EL_DEBUG == 0
        #define FDB_PRINT(...)
    #elif CONFIG_EL_DEBUG >= 1
        #define FDB_DEBUG_ENABLE
    #endif

#endif

#endif
