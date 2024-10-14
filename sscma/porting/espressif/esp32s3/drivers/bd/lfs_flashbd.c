
#include <bd/lfs_flashbd.h>
#include <esp_partition.h>
#include <spi_flash_mmap.h>

const static esp_partition_t* _partition = NULL;

int lfs_flashbd_create(const struct lfs_config* cfg, const struct lfs_flashbd_config* bdcfg) {
    LFS_ASSERT(cfg && bdcfg);

    LFS_FLASHBD_TRACE(
        "lfs_flashbd_create(%p {.context=%p, "
        ".read=%p, .prog=%p, .erase=%p, .sync=%p}, "
        "%p {.read_size=%" PRIu32 ", .prog_size=%" PRIu32
        ", "
        ".erase_size=%" PRIu32 ", .erase_count=%" PRIu32
        ", "
        ".flash_addr=%p})",
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
        bdcfg->flash_addr);

    lfs_flashbd_t* bd = cfg->context;
    bd->cfg           = bdcfg;

    if (_partition) {
        LFS_ASSERT(!_partition && "Partition already exists");
    }

    _partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_UNDEFINED, "db");

    if (!_partition) {
        LFS_ASSERT(_partition && "Partition not found");
        return LFS_ERR_INVAL;
    }

    bd->flash_addr = _partition->address;

    LFS_FLASHBD_TRACE("lfs_flashbd_create -> %d", success ? 0 : LFS_ERR_IO);

    return 0;
}

int lfs_flashbd_destroy(const struct lfs_config* cfg) {
    LFS_FLASHBD_TRACE("lfs_flashbd_destroy(%p)", (void*)cfg);
    LFS_FLASHBD_TRACE("lfs_flashbd_destroy -> %d", 0);
    return 0;
}

int lfs_flashbd_read(const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size) {
    LFS_FLASHBD_TRACE(
        "lfs_flashbd_read(%p, "
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

    int ret = esp_partition_read(_partition, block * bd->cfg->erase_size + off, buffer, size);

    LFS_FLASHBD_TRACE("lfs_flashbd_read -> %d", size);
    return ret == ESP_OK ? 0 : LFS_ERR_IO;
}

int lfs_flashbd_prog(const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size) {
    LFS_FLASHBD_TRACE(
        "lfs_flashbd_prog(%p, "
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

    int ret = esp_partition_write(_partition, block * bd->cfg->erase_size + off, buffer, size);

    LFS_FLASHBD_TRACE("lfs_flashbd_prog");
    return ret == ESP_OK ? 0 : LFS_ERR_IO;
}

int lfs_flashbd_erase(const struct lfs_config* cfg, lfs_block_t block) {
    LFS_FLASHBD_TRACE("lfs_flashbd_erase(%p, 0x%" PRIx32 " (%" PRIu32 "))", (void*)cfg, block, ((lfs_flashbd_t*)cfg->context)->cfg->erase_size);
    lfs_flashbd_t* bd = cfg->context;

    // check if erase is valid
    LFS_ASSERT(block < bd->cfg->erase_count);

    int ret = esp_partition_erase_range(_partition, block * bd->cfg->erase_size, bd->cfg->erase_size);

    LFS_FLASHBD_TRACE("lfs_flashbd_erase -> %d", ret == ESP_OK ? 0 : LFS_ERR_IO);
    return ret == ESP_OK ? 0 : LFS_ERR_IO;
}

int lfs_flashbd_sync(const struct lfs_config* cfg) {
    LFS_FLASHBD_TRACE("lfs_flashbd_sync(%p)", (void*)cfg);

    (void)cfg;

    LFS_FLASHBD_TRACE("lfs_flashbd_sync -> %d", 0);
    return 0;
}
