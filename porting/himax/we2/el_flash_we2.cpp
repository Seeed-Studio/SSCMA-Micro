/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 (Seeed Technology Inc.)
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

#include "core/el_debug.h"
#include "core/el_types.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "el_config_porting.h"
#include "spi_eeprom_comm.h"

namespace edgelab {

#if CONFIG_EL_MODEL
namespace internal {

static void enable_we2_xip() {
    static int once_please = []() {
        hx_lib_spi_eeprom_open(USE_DW_SPI_MST_Q);
        hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true);
        return 0;
    }();
}

el_err_code_t el_model_partition_mmap_init(uint32_t*       partition_start_addr,
                                           uint32_t*       partition_size,
                                           const uint8_t** flash_2_memory_map,
                                           uint32_t*       mmap_handler) {
    *partition_start_addr = 0x00000000;
    *partition_size       = 0x00000010;
    enable_we2_xip();
    *flash_2_memory_map = (const uint8_t*)0x3A400000;

    return EL_OK;
}

void el_model_partition_mmap_deinit(uint32_t* mmap_handler) {
    hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, false, FLASH_QUAD, true);
}

}  // namespace internal
#endif

#ifdef CONFIG_EL_LIB_FLASHDB
static Mutex        el_flash_db_lock{};
const static size_t el_flash_db_partition_end = 0x00800000;
const static size_t el_flash_db_partition     = el_flash_db_partition_end - CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0;

static int el_flash_db_init(void) {
    enable_we2_xip();
    return 1;
}

static int el_flash_db_read(long offset, uint8_t* buf, size_t size) {
    const Guard<Mutex> guard(el_flash_db_lock);
    int8_t             ret  = 0;
    uint32_t           addr = el_flash_db_nor_flash0.addr + offset;

    if (addr + size > el_flash_db_partition_end) [[unlikely]]
        return -1;

    memcpy(buf, (uint8_t*)(0x3A000000 + addr), size);

    return ret;
}

static int el_flash_db_write(long offset, const uint8_t* buf, size_t size) {
    const Guard<Mutex> guard(el_flash_db_lock);
    int8_t             ret  = 0;
    uint32_t           addr = el_flash_db_nor_flash0.addr + offset;

    if (addr + size > el_flash_db_partition_end) [[unlikely]]
        return -1;

    hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, false, FLASH_QUAD, false);
    hx_lib_spi_eeprom_word_write(USE_DW_SPI_MST_Q, addr, (uint32_t*)buf, size);
    hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true);

    return ret;
}

static int el_flash_db_erase(long offset, size_t size) {
    const Guard<Mutex> guard(el_flash_db_lock);
    int8_t             ret  = 0;
    uint32_t           addr = el_flash_db_nor_flash0.addr + offset;

    if (addr + size > el_flash_db_partition_end) [[unlikely]]
        return -1;

    hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, false, FLASH_QUAD, false);
    hx_lib_spi_eeprom_erase_sector(USE_DW_SPI_MST_Q, addr, FLASH_SECTOR);
    hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true);

    return ret;
}

}  // namespace edgelab

    #if CONFIG_EL_LIB_FLASHDB
        #ifdef __cpluscplus
extern "C" {
        #endif

const struct fal_flash_dev el_flash_db_nor_flash0 {
    .name = CONFIG_EL_STORAGE_PARTITION_MOUNT_POINT, .addr = el_flash_db_partition,
    .len = CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0, .blk_size = FDB_BLOCK_SIZE,
    .ops        = {edgelab::el_flash_db_init,
                   edgelab::el_flash_db_read,
                   edgelab::el_flash_db_write,
                   edgelab::el_flash_db_erase},
    .write_gran = FDB_WRITE_GRAN,
};

        #ifdef __cpluscplus
}
        #endif
    #endif
