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

extern "C" {
#include <spi_eeprom_comm.h>
}

#include <cstdint>
#include <cstring>

#include "core/el_debug.h"
#include "core/el_types.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "el_config_porting.h"
#include "porting/el_flash.h"

namespace edgelab {

namespace porting {

static Mutex         _el_flash_lock{};
static volatile bool _el_flash_enabled = false;
static volatile bool _el_xip_enabled   = false;

bool _el_flash_init() {
    const Guard<Mutex> guard(_el_flash_lock);

    if (_el_flash_enabled) [[unlikely]]
        goto Return;

    if (hx_lib_spi_eeprom_open(USE_DW_SPI_MST_Q) != E_OK) [[unlikely]]
        goto Return;

    _el_flash_enabled = true;

Return:
    return _el_flash_enabled;
}

void _el_flash_deinit() {}

bool _el_flash_enable_xip() {
    if (!_el_xip_enabled) [[likely]] {
        if (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true) == E_OK) [[likely]]
            _el_xip_enabled = true;
        else
            return false;
    }

    return true;
}

bool el_flash_mmap_init(uint32_t* flash_addr, uint32_t* size, const uint8_t** mmap, uint32_t* handler) {
    *flash_addr = 0x00400000;
    *size = 0x00A00000;

    if (!_el_flash_init()) [[unlikely]]
        return false;

    const Guard<Mutex> guard(_el_flash_lock);

    if (!_el_xip_enabled) [[likely]] {
        if (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true) == E_OK) [[likely]]
            _el_xip_enabled = true;
        else
            return false;
    }

    *mmap = reinterpret_cast<const uint8_t*>(0x3A000000 + *flash_addr);

    return true;
}

void el_flash_mmap_deinit(uint32_t*) { _el_flash_deinit(); }

#ifdef CONFIG_EL_LIB_FLASHDB

const static size_t _el_flash_db_partition_begin = 0x00300000;
const static size_t _el_flash_db_partition_end   = 0x00300000 + CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0;

static int _el_flash_db_init(void) {
    if (!_el_flash_init()) [[unlikely]]
        return -1;

    return 1;
}

static int _el_flash_db_read(long offset, uint8_t* buf, size_t size) {
    const Guard<Mutex> guard(_el_flash_lock);

    hx_drv_watchdog_update(WATCHDOG_ID_0, WATCH_DOG_TIMEOUT_TH);
    uint32_t addr = _el_flash_db_nor_flash0.addr + offset;
    if (addr + size > _el_flash_db_partition_end) [[unlikely]]
        return -1;

    if (!_el_xip_enabled) [[unlikely]] {
        if (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true) == E_OK) [[likely]]
            _el_xip_enabled = true;
        else
            return -1;
    }

    // use xip to read data
    std::memcpy(buf, reinterpret_cast<uint8_t*>(0x3A000000 + addr), size);

    return 0;
}

static int _el_flash_db_write(long offset, const uint8_t* buf, size_t size) {
    const Guard<Mutex> guard(_el_flash_lock);

    uint32_t addr = _el_flash_db_nor_flash0.addr + offset;

    hx_drv_watchdog_update(WATCHDOG_ID_0, WATCH_DOG_TIMEOUT_TH);

    if (addr + size > _el_flash_db_partition_end) [[unlikely]]
        return -1;

    if (_el_xip_enabled) [[likely]] {
        if (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, false, FLASH_QUAD, false) == E_OK) [[likely]]
            _el_xip_enabled = false;
        else
            return -1;
    }

    int ret = -1;
    if (hx_lib_spi_eeprom_word_write(
          USE_DW_SPI_MST_Q, addr, const_cast<uint32_t*>(reinterpret_cast<const uint32_t*>(buf)), size) == E_OK)
      [[likely]]
        ret = 0;

    if (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true) == E_OK) [[likely]]
        _el_xip_enabled = true;

    return ret;
}

static int _el_flash_db_erase(long offset, size_t size) {
    const Guard<Mutex> guard(_el_flash_lock);

    hx_drv_watchdog_update(WATCHDOG_ID_0, WATCH_DOG_TIMEOUT_TH);

    uint32_t addr = _el_flash_db_nor_flash0.addr + offset;

    if (addr + size > _el_flash_db_partition_end) [[unlikely]]
        return -1;

    if (_el_xip_enabled) {
        if (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, false, FLASH_QUAD, false) == E_OK) [[likely]]
            _el_xip_enabled = false;
        else
            return -1;
    }

    int ret = -1;
    for (size_t i = addr; i < addr + size; i += 4096) {
        ret = -1;
        if (i < _el_flash_db_partition_begin) continue;
        if (i >= _el_flash_db_partition_end) break;
        hx_drv_watchdog_update(WATCHDOG_ID_0, WATCH_DOG_TIMEOUT_TH);
        if (hx_lib_spi_eeprom_erase_sector(USE_DW_SPI_MST_Q, i, FLASH_SECTOR) == E_OK) [[likely]]
            ret = 0;
    }

    if (hx_lib_spi_eeprom_enable_XIP(USE_DW_SPI_MST_Q, true, FLASH_QUAD, true) == E_OK) [[likely]]
        _el_xip_enabled = true;

    return ret;
}

extern "C" const struct fal_flash_dev _el_flash_db_nor_flash0 = {
  .name       = CONFIG_EL_STORAGE_PARTITION_MOUNT_POINT,
  .addr       = _el_flash_db_partition_begin,
  .len        = CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0,
  .blk_size   = FDB_BLOCK_SIZE,
  .ops        = {_el_flash_db_init, _el_flash_db_read, _el_flash_db_write, _el_flash_db_erase},
  .write_gran = FDB_WRITE_GRAN,
};

#endif

}  // namespace porting

}  // namespace edgelab
