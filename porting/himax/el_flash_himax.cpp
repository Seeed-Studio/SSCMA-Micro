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

#include "el_flash_himax.h"

#include <internal_flash.h>

#include "core/el_debug.h"

namespace edgelab {

el_err_code_t el_model_partition_mmap_init(const char*              partition_name,
                                           uint32_t*                partition_start_addr,
                                           uint32_t*                partition_size,
                                           const uint8_t**          flash_2_memory_map,
                                           el_model_mmap_handler_t* mmap_handler) {
    return EL_OK;
}

void el_model_partition_mmap_deinit(el_model_mmap_handler_t* mmap_handler) {}

static int el_flash_db_init(void) { return 1; }

static int el_flash_db_read(long offset, uint8_t* buf, size_t size) {
    int8_t   ret  = 0;
    uint32_t addr = el_flash_db_nor_flash0.addr + offset;

    ret = internal_flash_read(addr, buf, size);
    //EMBARC_PRINTF("flash db read 0x%08x 0x%08x str:%s\n", addr, size, buf);
    return ret;
}

static int el_flash_db_write(long offset, const uint8_t* buf, size_t size) {
    int8_t   ret  = 0;
    uint32_t addr = el_flash_db_nor_flash0.addr + offset;
    //EMBARC_PRINTF("flash db write 0x%08x 0x%08x str:%s\n", addr, size, buf);
    ret = internal_flash_write(addr, (void*)buf, size);

    return ret;
}

static int el_flash_db_erase(long offset, size_t size) {
    int8_t   ret  = 0;
    uint32_t addr = el_flash_db_nor_flash0.addr + offset;
    //EMBARC_PRINTF("flash db erase 0x%08x 0x%08x\n", addr, size);
    ret = internal_flash_clear(addr, size);

    return ret;
}

#ifdef CONFIG_EL_LIB_FLASHDB
const struct fal_flash_dev el_flash_db_nor_flash0 = {
  .name = CONFIG_EL_STORAGE_PARTITION_MOUNT_POINT,
 //0x200000 is the flash size of himax6538, 0x10000 is the data size for flashdb
  .addr       = (0x200000 - 0x10000),
  .len        = 0x10000,
  .blk_size   = FDB_BLOCK_SIZE,
  .ops        = {el_flash_db_init, el_flash_db_read, el_flash_db_write, el_flash_db_erase},
  .write_gran = FDB_WRITE_GRAN,
};
#endif

}  // namespace edgelab
