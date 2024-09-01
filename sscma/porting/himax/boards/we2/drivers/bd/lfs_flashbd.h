
#ifndef LFS_FLASHBD_H
#define LFS_FLASHBD_H

#include "lfs.h"
#include "lfs_util.h"

#ifdef __cplusplus
extern "C" {
#endif

// Block device specific tracing
#ifndef LFS_FLASHBD_TRACE
    #ifdef LFS_FLASHBD_YES_TRACE
        #define LFS_FLASHBD_TRACE(...) LFS_TRACE(__VA_ARGS__)
    #else
        #define LFS_FLASHBD_TRACE(...)
    #endif
#endif

// flashbd config
struct lfs_flashbd_config {
    // Minimum size of a read operation in bytes.
    lfs_size_t read_size;

    // Minimum size of a program operation in bytes.
    lfs_size_t prog_size;

    // Size of an erase operation in bytes.
    lfs_size_t erase_size;

    // Number of erase blocks on the device.
    lfs_size_t erase_count;

    lfs_size_t flash_size;

    void* flash_addr;
};

// flashbd state
typedef struct lfs_flashbd {
    void*                            flash_addr;
    const struct lfs_flashbd_config* cfg;
} lfs_flashbd_t;

int lfs_flashbd_create(const struct lfs_config* cfg, const struct lfs_flashbd_config* bdcfg);

int lfs_flashbd_destroy(const struct lfs_config* cfg);

// Read a block
int lfs_flashbd_read(const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);

// Program a block
//
// The block must have previously been erased.
int lfs_flashbd_prog(
  const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);

// Erase a block
//
// A block must be erased before being programmed. The
// state of an erased block is undefined.
int lfs_flashbd_erase(const struct lfs_config* cfg, lfs_block_t block);

// Sync the block device
int lfs_flashbd_sync(const struct lfs_config* cfg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
