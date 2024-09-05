
#ifndef _LFS_FLASHBD_H_
#define _LFS_FLASHBD_H_

#include "lfs.h"
#include "lfs_util.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LFS_FLASHBD_TRACE
    #ifdef LFS_FLASHBD_YES_TRACE
        #define LFS_FLASHBD_TRACE(...) LFS_TRACE(__VA_ARGS__)
    #else
        #define LFS_FLASHBD_TRACE(...)
    #endif
#endif

struct lfs_flashbd_config {
    lfs_size_t read_size;
    lfs_size_t prog_size;
    lfs_size_t erase_size;
    lfs_size_t erase_count;
    void*      flash_addr;
};

typedef struct lfs_flashbd {
    void*                            flash_addr;
    const struct lfs_flashbd_config* cfg;
} lfs_flashbd_t;

int lfs_flashbd_create(const struct lfs_config* cfg, const struct lfs_flashbd_config* bdcfg);

int lfs_flashbd_destroy(const struct lfs_config* cfg);

int lfs_flashbd_read(const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);

int lfs_flashbd_prog(
  const struct lfs_config* cfg, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size);

int lfs_flashbd_erase(const struct lfs_config* cfg, lfs_block_t block);

int lfs_flashbd_sync(const struct lfs_config* cfg);

#ifdef __cplusplus
}
#endif

#endif
