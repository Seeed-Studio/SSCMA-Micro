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

#include "el_flash_esp.h"

#include "core/el_debug.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"

namespace edgelab {

static Mutex                  el_flash_db_lock;
const static esp_partition_t* el_flash_db_partition = nullptr;

el_err_code_t el_model_partition_mmap_init(const char*              partition_name,
                                           uint32_t*                partition_start_addr,
                                           uint32_t*                partition_size,
                                           const uint8_t**          flash_2_memory_map,
                                           spi_flash_mmap_handle_t* mmap_handler) {
    const esp_partition_t* partition{
      esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, partition_name)};
    if (!partition) [[unlikely]]
        return EL_EINVAL;

    *partition_start_addr = partition->address;
    *partition_size       = partition->size;

    esp_err_t ret = spi_flash_mmap(*partition_start_addr,
                                   *partition_size,
                                   SPI_FLASH_MMAP_DATA,
                                   reinterpret_cast<const void**>(flash_2_memory_map),
                                   mmap_handler);

    return ret != ESP_OK ? EL_EINVAL : EL_OK;
}

void el_model_partition_mmap_deinit(spi_flash_mmap_handle_t* mmap_handler) { spi_flash_munmap(*mmap_handler); }

static int el_flash_db_init(void) {
    el_flash_db_partition = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, CONFIG_EL_STORAGE_PARTITION_NAME);
    EL_ASSERT(el_flash_db_partition != nullptr);
    return 1;
}

static int el_flash_db_read(long offset, uint8_t* buf, size_t size) {
    const Guard<Mutex> guard(el_flash_db_lock);
    return esp_partition_read(el_flash_db_partition, offset, buf, size);
}

static int el_flash_db_write(long offset, const uint8_t* buf, size_t size) {
    const Guard<Mutex> guard(el_flash_db_lock);
    return esp_partition_write(el_flash_db_partition, offset, buf, size);
}

static int el_flash_db_erase(long offset, size_t size) {
    const Guard<Mutex> guard(el_flash_db_lock);
    int32_t            erase_size = ((size - 1) / FDB_BLOCK_SIZE) + 1;
    return esp_partition_erase_range(el_flash_db_partition, offset, erase_size * FDB_BLOCK_SIZE);
}

#ifdef CONFIG_EL_LIB_FLASHDB
const struct fal_flash_dev el_flash_db_nor_flash0 = {
  .name       = NOR_FLASH_DEV_NAME,
  .addr       = 0x0,
  .len        = 192 * 1024,
  .blk_size   = FDB_BLOCK_SIZE,
  .ops        = {el_flash_db_init, el_flash_db_read, el_flash_db_write, el_flash_db_erase},
  .write_gran = FDB_WRITE_GRAN,
};
#endif

}  // namespace edgelab
