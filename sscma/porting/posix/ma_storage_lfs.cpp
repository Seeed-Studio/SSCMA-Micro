#include "ma_storage_lfs.h"

#include <cctype>
#include <list>
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
static const char        hex_digits[]       = "0123456789abcdef";

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

ma_err_t StorageLfs::mount(bool force) {
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
        MA_LOGE(TAG, "Failed to create LFS storage %s", mount_point_.c_str());
        return MA_EIO;
    }

    ret = lfs_mount(&lfs_, &fs_config_);
    if (force && ret != LFS_ERR_OK) {
        ret = lfs_format(&lfs_, &fs_config_);
        if (ret != LFS_ERR_OK) {
            MA_LOGE(TAG, "Failed to format LFS storage");
            return MA_EIO;
        }
        ret = lfs_mount(&lfs_, &fs_config_);
    }
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

std::list<std::string> StorageLfs::keyToPath(const std::string& key, size_t break_size) {
    std::string encoded_key;
    for (const char c : key) {
        if (std::isalnum(c) || c == '_' || c == '-' || c == '.') {
            encoded_key.push_back(c);
        } else {
            char enc[3] = {
              '%',
              hex_digits[(c >> 4) & 0xF],
              hex_digits[c & 0xF],
            };
            encoded_key.append(enc, sizeof(enc));
        }
    }
    encoded_key.push_back('#');

    std::list<std::string> parts;
    for (size_t i = 0; i < encoded_key.size(); i += break_size) {
        parts.push_back(std::move(encoded_key.substr(i, break_size)));
    }
    return std::move(parts);
}

ma_err_t StorageLfs::setImpl(std::string key, void const* data, size_t size) {
    if (!is_mounted_) {
        return MA_EPERM;
    }

    static_assert(LFS_NAME_MAX > 16);
    auto parts = keyToPath(key, 16);
    if (parts.empty()) {
        return MA_EINVAL;
    }

    std::string path;
    for (auto it = parts.begin(); it != std::prev(parts.end()); ++it) {
        path.push_back('/');
        path += std::move(*it);
        int ret = lfs_mkdir(&lfs_, path.c_str());
        if (ret != LFS_ERR_OK && ret != LFS_ERR_EXIST) {
            MA_LOGE(TAG, "Failed to create directory %s", path.c_str());
            return MA_EIO;
        }
    }
    path.push_back('/');
    path += parts.back();
    parts.clear();

    lfs_file_t file;
    int        ret = lfs_file_open(&lfs_, &file, path.c_str(), LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to open file %s", path.c_str());
        return MA_EIO;
    }

    const auto written = lfs_file_write(&lfs_, &file, data, size);
    if (written != size) {
        MA_LOGE(TAG, "Failed to write to file %s", path.c_str());

        lfs_file_close(&lfs_, &file);

        return MA_EIO;
    }

    ret = lfs_file_close(&lfs_, &file);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to close file %s", path.c_str());
        return MA_EIO;
    }

    return MA_OK;
}

ma_err_t StorageLfs::getImpl(std::string key, std::string& buffer) {
    if (!is_mounted_) {
        return MA_EPERM;
    }

    static_assert(LFS_NAME_MAX > 16);
    auto parts = keyToPath(key, 16);
    if (parts.empty()) {
        return MA_EINVAL;
    }

    std::string path;
    for (auto& part : parts) {
        path.push_back('/');
        path += std::move(part);
    }
    parts.clear();

    lfs_file_t file;
    int        ret = lfs_file_open(&lfs_, &file, path.c_str(), LFS_O_RDONLY);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to open file %s, code %d", path.c_str(), ret);
        return MA_EIO;
    }

    lfs_info info;
    ret = lfs_stat(&lfs_, path.c_str(), &info);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to stat file %s", path.c_str());
        return MA_EIO;
    }
    const size_t size = info.size;
    buffer.resize(size);
    lfs_size_t read = lfs_file_read(&lfs_, &file, buffer.data(), size);
    if (read != size) {
        MA_LOGE(TAG, "Failed to read from file %s, code %d", path.c_str(), read);

        lfs_file_close(&lfs_, &file);

        return MA_EIO;
    }

    ret = lfs_file_close(&lfs_, &file);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to close file %s", path.c_str());
        return MA_EIO;
    }

    return MA_OK;
}

ma_err_t StorageLfs::remove(const std::string& key) {
    Guard guard(mutex_);

    if (!is_mounted_) {
        return MA_EPERM;
    }

    static_assert(LFS_NAME_MAX > 16);
    auto parts = keyToPath(key, 16);
    if (parts.empty()) {
        return MA_EINVAL;
    }

    std::string path;
    for (const auto& part : parts) {
        path.push_back('/');
        path += part;
    }

    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
        int ret = lfs_remove(&lfs_, path.c_str());
        if (ret == LFS_ERR_NOENT || ret == LFS_ERR_NOTEMPTY) {
            break;
        } else if (ret != LFS_ERR_OK) {
            MA_LOGE(TAG, "Failed to remove file %s", path.c_str());
            return MA_EIO;
        }

        path = path.substr(0, path.size() - it->size());
        path.pop_back();
    }

    return MA_OK;
}

bool StorageLfs::exists(const std::string& key) {
    Guard guard(mutex_);

    if (!is_mounted_) {
        return false;
    }

    static_assert(LFS_NAME_MAX > 16);
    auto parts = keyToPath(key, 16);
    if (parts.empty()) {
        return false;
    }

    std::string path;
    for (const auto& part : parts) {
        path.push_back('/');
        path += part;
    }

    lfs_info info;
    int      ret = lfs_stat(&lfs_, path.c_str(), &info);
    if (ret == LFS_ERR_OK) {
        return true;
    }

    return false;
}

ma_err_t StorageLfs::set(const std::string& key, const void* value, size_t size) {
    Guard guard(mutex_);
    return setImpl(key, value, size);
}

ma_err_t StorageLfs::get(const std::string& key, std::string& value) {
    Guard guard(mutex_);

    return getImpl(key, value);
}

}  // namespace ma
