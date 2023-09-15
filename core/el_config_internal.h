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
#endif

/* porting config check */
#ifndef CONFIG_EL_PORTING_POSIX
    #if defined(__unix__) || defined(__APPLE__)
        #define CONFIG_EL_PORTING_POSIX 1
    #else
        #define CONFIG_EL_PORTING_POSIX 0
    #endif
#endif

/* model related config */
#define CONFIG_EL_MODEL_TFLITE_MAGIC   0x54464C33
#define CONFIG_EL_MODEL_HEADER_MAGIC   0x4C4854
#define CONFIG_EL_MODEL_PARTITION_NAME "models"

/* storage related config */
#define CONFIG_EL_STORAGE_NAME                  "edgelab_db"
#define CONFIG_EL_STORAGE_PATH                  "kvdb0"
#define CONFIG_EL_STORAGE_PARTITION_NAME        "db"
#define CONFIG_EL_STORAGE_PARTITION_MOUNT_POINT "nor_flash0"
#define CONFIG_EL_STORAGE_PARTITION_FS_NAME_0   "kvdb0"
#define CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0   (192 * 1024)
#define CONFIG_EL_STORAGE_KEY_SIZE_MAX          (64)

#endif
