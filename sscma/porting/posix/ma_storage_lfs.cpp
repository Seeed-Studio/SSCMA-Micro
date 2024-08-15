#include "ma_storage_lfs.h"

#include <vector>

extern "C" {

#ifdef LFS_THREADSAFE

static ma::Mutex _lfs_llvl_mutex;

static int _lfs_llvl_lock(const struct lfs_config* c) {
    _lfs_llvl_mutex.lock();
    return LFS_ERR_OK;
}

static int _lfs_llvl_unlock(const struct lfs_config* c) {
    _lfs_llvl_mutex.unlock();
    return LFS_ERR_OK;
}

#endif
}

namespace ma {

static constexpr char TAG[] = "ma::storage::lfs";

static const std::string default_filename   = "sscma.lfs";
static const size_t      default_block_size = 4096;

StorageLfs::StorageLfs() : StorageLfs(default_filename) {}

StorageLfs::StorageLfs(const std::string& mount_point, size_t bd_size)
    : is_mounted_(false), mutex_(), mount_point_(mount_point), bd_size_(bd_size) {
#ifndef MA_STORAGE_NO_AUTO_MOUNT
    auto ret = mount();
    if (ret != MA_OK) {
        MA_LOGE(TAG, "Failed to mount LFS storage");
    }
#endif
}

StorageLfs::~StorageLfs() {
#ifndef MA_STORAGE_NO_AUTO_MOUNT
    auto ret = uMount();
    if (ret != MA_OK) {
        MA_LOGE(TAG, "Failed to unmount LFS storage");
    }
#endif
}

ma_err_t StorageLfs::mount() {
    Guard guard(mutex_);

    if (mount_point_.empty()) {
        return MA_EINVAL;
    }

    if (is_mounted_) {
        return MA_OK;
    }

    const uint32_t block_count = bd_size_ / default_block_size;
    if (block_count == 0) {
        return MA_EINVAL;
    }

    bd_config_.read_size   = 16;
    bd_config_.prog_size   = 16;
    bd_config_.erase_size  = default_block_size;
    bd_config_.erase_count = block_count;

    fs_config_ = lfs_config{
      .context = &bd_,
      .read    = lfs_filebd_read,
      .prog    = lfs_filebd_prog,
      .erase   = lfs_filebd_erase,
      .sync    = lfs_filebd_sync,
#ifdef LFS_THREADSAFE
      .lock   = _lfs_llvl_lock,
      .unlock = _lfs_llvl_unlock,
#endif
      .read_size        = bd_config_.read_size,
      .prog_size        = bd_config_.prog_size,
      .block_size       = default_block_size,
      .block_count      = block_count,
      .block_cycles     = 500,
      .cache_size       = 16,
      .lookahead_size   = 16,
      .read_buffer      = nullptr,
      .prog_buffer      = nullptr,
      .lookahead_buffer = nullptr,
      .name_max         = 255,
    };

    int ret = lfs_filebd_create(&fs_config_, mount_point_.c_str(), &bd_config_);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to create LFS storage");
        return MA_EIO;
    }

    ret = lfs_mount(&lfs_, &fs_config_);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to mount LFS storage");
        return MA_EIO;
    }

    is_mounted_ = true;

    return MA_OK;
}

ma_err_t StorageLfs::uMount() {
    Guard guard(mutex_);

    if (!is_mounted_) {
        return MA_OK;
    }

    int ret = lfs_unmount(&lfs_);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to unmount LFS storage");
        return MA_EIO;
    }

    ret = lfs_filebd_destroy(&fs_config_);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to destroy LFS storage");
        return MA_EIO;
    }

    is_mounted_ = false;

    return MA_OK;
}

}  // namespace ma