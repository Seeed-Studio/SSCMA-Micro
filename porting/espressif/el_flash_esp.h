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

#ifndef _EL_FLASH_ESP_H_
#define _EL_FLASH_ESP_H_

#include <esp_partition.h>
#include <spi_flash_mmap.h>

#include "core/el_config_internal.h"
#include "core/el_types.h"

#ifdef __cplusplus
namespace edgelab {
extern "C" {
#endif

typedef spi_flash_mmap_handle_t el_model_mmap_handler_t;

el_err_code_t el_model_partition_mmap_init(const char*              partition_name,
                                           uint32_t*                partition_start_addr,
                                           uint32_t*                partition_size,
                                           const uint8_t**          flash_2_memory_map,
                                           spi_flash_mmap_handle_t* mmap_handler);

void el_model_partition_mmap_deinit(spi_flash_mmap_handle_t* mmap_handler);

#ifdef CONFIG_EL_LIB_FLASHDB
    #include "third_party/FlashDB/fal_def.h"

    #define NOR_FLASH_DEV_NAME CONFIG_EL_STORAGE_PARTITION_MOUNT_POINT
    #define FAL_FLASH_DEV_TABLE \
        { &el_flash_db_nor_flash0, }

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
    #define FDB_WRITE_GRAN (1)
    #define FDB_BLOCK_SIZE (8 * 1024)

    #if CONFIG_EL_DEBUG >= 1
        #define FDB_DEBUG_ENABLE
    #endif

extern const struct fal_flash_dev el_flash_db_nor_flash0;

#endif

#ifdef __cplusplus
}
}
#endif

#endif
