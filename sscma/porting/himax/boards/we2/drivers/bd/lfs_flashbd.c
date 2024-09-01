
#include "lfs_flashbd.h"

#include <spi_eeprom_comm.h>

#ifndef LFS_FLASHBD_CREATE_RETRY
    #define LFS_FLASHBD_CREATE_RETRY 10
#endif

#include "xip.h"

int lfs_flashbd_create(const struct lfs_config* cfg, const struct lfs_flashbd_config* bdcfg) {
    LFS_FLASHBD_TRACE("lfs_flashbd_create(%p {.context=%p, "
                      ".read=%p, .prog=%p, .erase=%p, .sync=%p}, "
                      "%p {.read_size=%" PRIu32 ", .prog_size=%" PRIu32 ", "
                      ".erase_size=%" PRIu32 ", .erase_count=%" PRIu32 ", "
                      ".buffer=%p})",
                      (void*)cfg,
                      cfg->context,
                      (void*)(uintptr_t)cfg->read,
                      (void*)(uintptr_t)cfg->prog,
                      (void*)(uintptr_t)cfg->erase,
                      (void*)(uintptr_t)cfg->sync,
                      (void*)bdcfg,
                      bdcfg->read_size,
                      bdcfg->prog_size,
                      bdcfg->erase_size,
                      bdcfg->erase_count,
                      bdcfg->buffer);

    lfs_flashbd_t* bd = cfg->context;
    bd->cfg           = bdcfg;

    if (bd->cfg->flash_addr) {
        bd->flash_addr = bd->cfg->flash_addr;
    } else {
        LFS_ASSERT(bd->cfg->flash_addr && "flash_addr must be provided");
        return LFS_ERR_INVAL;
    }

    int retry = LFS_FLASHBD_CREATE_RETRY;
    xip_ownership_acquire();
    while (hx_lib_spi_eeprom_open(USE_DW_SPI_MST_Q) != E_OK) {
        xip_ownership_release();
        if (--retry < 0) {
            LFS_FLASHBD_TRACE("lfs_flashbd_create -> %d", LFS_ERR_IO);
            return LFS_ERR_IO;
        }
    }

    LFS_FLASHBD_TRACE("lfs_flashbd_create -> %d", 0);

    return 0;
}

int lfs_flashbd_destroy(const struct lfs_config* cfg) {
    LFS_FLASHBD_TRACE("lfs_flashbd_destroy(%p)", (void*)cfg);
    LFS_FLASHBD_TRACE("lfs_flashbd_destroy -> %d", 0);
    return 0;
}

int lfs_flashbd_read(const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size) {
    LFS_FLASHBD_TRACE("lfs_flashbd_read(%p, "
                      "0x%" PRIx32 ", %" PRIu32 ", %p, %" PRIu32 ")",
                      (void*)cfg,
                      block,
                      off,
                      buffer,
                      size);
    lfs_flashbd_t* bd = cfg->context;

    // check if read is valid
    LFS_ASSERT(block < bd->cfg->erase_count);
    LFS_ASSERT(off % bd->cfg->read_size == 0);
    LFS_ASSERT(size % bd->cfg->read_size == 0);
    LFS_ASSERT(off + size <= bd->cfg->erase_size);

    xip_ownership_acquire();

    if (!xip_safe_enable()) {
        LFS_FLASHBD_TRACE("lfs_flashbd_read -> %d", LFS_ERR_IO);
        xip_ownership_release();
        return LFS_ERR_IO;
    }

    // read data
    memcpy(buffer, &bd->flash_addr[block * bd->cfg->erase_size + off], size);

    xip_ownership_release();

    LFS_FLASHBD_TRACE("lfs_flashbd_read -> %d", 0);
    return 0;
}

int lfs_flashbd_prog(
  const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size) {
    LFS_FLASHBD_TRACE("lfs_flashbd_prog(%p, "
                      "0x%" PRIx32 ", %" PRIu32 ", %p, %" PRIu32 ")",
                      (void*)cfg,
                      block,
                      off,
                      buffer,
                      size);
    lfs_flashbd_t* bd = cfg->context;

    // check if write is valid
    LFS_ASSERT(block < bd->cfg->erase_count);
    LFS_ASSERT(off % bd->cfg->prog_size == 0);
    LFS_ASSERT(size % bd->cfg->prog_size == 0);
    LFS_ASSERT(off + size <= bd->cfg->erase_size);

    xip_ownership_acquire();

    if (!xip_safe_disable()) {
        LFS_FLASHBD_TRACE("lfs_flashbd_read -> %d", LFS_ERR_IO);
        xip_ownership_release();
        return LFS_ERR_IO;
    }

    int ret = (hx_lib_spi_eeprom_word_write(
                 USE_DW_SPI_MST_Q, &bd->flash_addr[block * bd->cfg->erase_size + off], buffer, size) == E_OK)
                ? 0
                : LFS_ERR_IO;

    xip_ownership_release();

    LFS_FLASHBD_TRACE("lfs_flashbd_prog -> %d", ret);
    return ret;
}

int lfs_flashbd_erase(const struct lfs_config* cfg, lfs_block_t block) {
    LFS_FLASHBD_TRACE("lfs_flashbd_erase(%p, 0x%" PRIx32 " (%" PRIu32 "))",
                      (void*)cfg,
                      block,
                      ((lfs_flashbd_t*)cfg->context)->cfg->erase_size);
    lfs_flashbd_t* bd = cfg->context;

    // check if erase is valid
    LFS_ASSERT(block < bd->cfg->erase_count);
    LFS_ASSERT(block == 4096);

    xip_ownership_acquire();

    if (!xip_safe_disable()) {
        LFS_FLASHBD_TRACE("lfs_flashbd_read -> %d", LFS_ERR_IO);
        xip_ownership_release();
        return LFS_ERR_IO;
    }

    int i = LFS_FLASHBD_CREATE_RETRY;
    while (hx_lib_spi_eeprom_erase_sector(USE_DW_SPI_MST_Q, i, FLASH_SECTOR) != E_OK) {
        if (--i < 0) {
            LFS_FLASHBD_TRACE("lfs_flashbd_erase -> %d", LFS_ERR_IO);
            xip_ownership_release();
            return LFS_ERR_IO;
        }
    }

    xip_ownership_release();

    LFS_FLASHBD_TRACE("lfs_flashbd_erase -> %d", 0);
    return 0;
}

int lfs_flashbd_sync(const struct lfs_config* cfg) {
    LFS_FLASHBD_TRACE("lfs_flashbd_sync(%p)", (void*)cfg);

    // sync is a noop
    (void)cfg;

    LFS_FLASHBD_TRACE("lfs_flashbd_sync -> %d", 0);
    return 0;
}
