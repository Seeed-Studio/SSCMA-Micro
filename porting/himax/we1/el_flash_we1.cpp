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
#include <internal_flash.h>
}

#include "core/el_debug.h"
#include "core/el_types.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "el_config_porting.h"

namespace edgelab {

namespace porting {

#if CONFIG_EL_MODEL

el_err_code_t _el_model_partition_mmap_init(uint32_t*       partition_start_addr,
                                            uint32_t*       partition_size,
                                            const uint8_t** flash_2_memory_map,
                                            uint32_t*       mmap_handler) {
    return EL_OK;
}

void _el_model_partition_mmap_deinit(uint32_t* mmap_handler) {}

#endif

#ifdef CONFIG_EL_LIB_FLASHDB

static Mutex        _el_flash_lock{};
const static size_t _el_flash_db_partition_end = 0x00200000;
const static size_t _el_flash_db_partition     = _el_flash_db_partition_end - CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0;

static int _el_flash_db_init(void) { return 1; }

static int _el_flash_db_read(long offset, uint8_t* buf, size_t size) {
    const Guard<Mutex> guard(_el_flash_lock);
    int8_t             ret  = 0;
    uint32_t           addr = _el_flash_db_nor_flash0.addr + offset;

    if (addr + size > _el_flash_db_partition_end) [[unlikely]]
        return -1;

    ret = internal_flash_read(addr, buf, size);

    return ret;
}

static int _el_flash_db_write(long offset, const uint8_t* buf, size_t size) {
    const Guard<Mutex> guard(_el_flash_lock);
    int8_t             ret  = 0;
    uint32_t           addr = _el_flash_db_nor_flash0.addr + offset;

    if (addr + size > _el_flash_db_partition_end) [[unlikely]]
        return -1;

    ret = internal_flash_write(addr, (void*)buf, size);

    return ret;
}

static int _el_flash_db_erase(long offset, size_t size) {
    const Guard<Mutex> guard(_el_flash_lock);
    int8_t             ret  = 0;
    uint32_t           addr = _el_flash_db_nor_flash0.addr + offset;

    if (addr + size > _el_flash_db_partition_end) [[unlikely]]
        return -1;

    ret = internal_flash_clear(addr, size);

    return ret;
}

extern "C" const struct fal_flash_dev _el_flash_db_nor_flash0 = {
  .name       = CONFIG_EL_STORAGE_PARTITION_MOUNT_POINT,
  .addr       = _el_flash_db_partition,
  .len        = CONFIG_EL_STORAGE_PARTITION_FS_SIZE_0,
  .blk_size   = FDB_BLOCK_SIZE,
  .ops        = {_el_flash_db_init, _el_flash_db_read, _el_flash_db_write, _el_flash_db_erase},
  .write_gran = FDB_WRITE_GRAN,
};

#endif

}  // namespace porting

}  // namespace edgelab
