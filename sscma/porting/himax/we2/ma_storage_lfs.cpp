#ifdef MA_FILESYSTEM_LITTLEFS

    #include "ma_storage_lfs.h"

    #include <cctype>
    #include <list>
    #include <vector>

    #include "core/ma_debug.h"

    #ifdef MA_STORAGE_LFS_NO_FORCE_MOUNT
        #define __FORCE_MOUNT false
    #else
        #define __FORCE_MOUNT true
    #endif

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

static constexpr char TAG[]        = "ma::storage::lfs";
static const char     hex_digits[] = "0123456789abcdef";

StorageLfs::StorageLfs() : Storage(), m_mutex() {}

StorageLfs::~StorageLfs() { deInit(); }

ma_err_t StorageLfs::init(const void* config) {
    Guard guard(m_mutex);

    if (m_initialized) [[unlikely]] {
        return MA_OK;
    }

    if (!config) [[unlikely]] {
        return MA_EINVAL;
    }

    #ifdef MA_STORAGE_LFS_USE_FLASHBD
    const auto* bd_config = reinterpret_cast<const ma_lfs_bd_cfg_t*>(config);
    #elif defined(MA_STORAGE_LFS_USE_FILEBD)
    const auto* bd_config = reinterpret_cast<const ma_lfs_bd_cfg_t*>(config);
    #elif defined(MA_STORAGE_LFS_USE_RAMBD)
    const auto* bd_config = reinterpret_cast<const ma_lfs_bd_cfg_t*>(config);
    #else
        #error "No block device specified"
    #endif

    m_fs_config = lfs_config{
      .context = &m_bd,
    #ifdef MA_STORAGE_LFS_USE_FLASHBD
      .read  = lfs_flashbd_read,
      .prog  = lfs_flashbd_prog,
      .erase = lfs_flashbd_erase,
      .sync  = lfs_flashbd_sync,
    #elif defined(MA_STORAGE_LFS_USE_FILEBD)
      .read  = lfs_filebd_read,
      .prog  = lfs_filebd_prog,
      .erase = lfs_filebd_erase,
      .sync  = lfs_filebd_sync,
    #elif defined(MA_STORAGE_LFS_USE_RAMBD)
      .read  = lfs_rambd_read,
      .prog  = lfs_rambd_prog,
      .erase = lfs_rambd_erase,
      .sync  = lfs_rambd_sync,
    #endif

    #ifdef LFS_THREADSAFE
      .lock   = _lfs_llvl_lock,
      .unlock = _lfs_llvl_unlock,
    #endif
      .read_size        = bd_config->read_size,
      .prog_size        = bd_config->prog_size,
      .block_size       = bd_config->erase_size,
      .block_count      = bd_config->erase_count,
      .block_cycles     = 1000,
      .cache_size       = 16,
      .lookahead_size   = 16,
      .read_buffer      = nullptr,
      .prog_buffer      = nullptr,
      .lookahead_buffer = nullptr,
      .name_max         = 255,
    };

    #ifdef MA_STORAGE_LFS_USE_FLASHBD
    int ret = lfs_flashbd_create(&m_fs_config, bd_config);
    #elif defined(MA_STORAGE_LFS_USE_FILEBD)
    int ret = lfs_filebd_create(&m_fs_config, MA_STORAGE_LFS_FILEBD_PATH, bd_config);
    #elif defined(MA_STORAGE_LFS_USE_RAMBD)
    int ret = lfs_rambd_create(&m_fs_config, bd_config);
    #endif
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to create LFS storage");
        return MA_EIO;
    }

    ret = lfs_mount(&m_lfs, &m_fs_config);
    if (__FORCE_MOUNT && ret != LFS_ERR_OK) {
        ret = lfs_format(&m_lfs, &m_fs_config);
        if (ret != LFS_ERR_OK) {
            MA_LOGE(TAG, "Failed to format LFS storage");
            return MA_EIO;
        }
        ret = lfs_mount(&m_lfs, &m_fs_config);
    }
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to mount LFS storage");
        return MA_EIO;
    }

    m_initialized = true;

    return MA_OK;
}

void StorageLfs::deInit() {
    Guard guard(m_mutex);

    if (!m_initialized) {
        return;
    }

    int ret = lfs_unmount(&m_lfs);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to unmount LFS storage");
    }

    ret = ma_lfs_bd_destory(&m_fs_config);
    if (ret != LFS_ERR_OK) {
        MA_LOGE(TAG, "Failed to destroy LFS storage");
    }

    m_initialized = false;
}

std::list<std::string> StorageLfs::keyToPath(const std::string& key, size_t break_size) {
    std::string encoded_key;
    for (const char c : key) {
        if (std::isalnum(c) || c == '_' || c == '-' || c == '.') [[likely]] {
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
    if (!m_initialized) [[unlikely]] {
        return MA_EPERM;
    }

    static_assert(LFS_NAME_MAX > 16);
    auto parts = keyToPath(key, 16);
    if (parts.empty()) [[unlikely]] {
        return MA_EINVAL;
    }

    std::string path;
    for (auto it = parts.begin(); it != std::prev(parts.end()); ++it) {
        path.push_back('/');
        path += std::move(*it);
        int ret = lfs_mkdir(&m_lfs, path.c_str());
        if (ret != LFS_ERR_OK && ret != LFS_ERR_EXIST) [[unlikely]] {
            MA_LOGE(TAG, "Failed to create directory %s", path.c_str());
            return MA_EIO;
        }
    }
    path.push_back('/');
    path += parts.back();
    parts.clear();

    lfs_file_t file;
    int        ret = lfs_file_open(&m_lfs, &file, path.c_str(), LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC);
    if (ret != LFS_ERR_OK) [[unlikely]] {
        MA_LOGE(TAG, "Failed to open file %s", path.c_str());
        return MA_EIO;
    }

    const auto written = lfs_file_write(&m_lfs, &file, data, size);
    if (written != size) [[unlikely]] {
        MA_LOGE(TAG, "Failed to write to file %s", path.c_str());

        lfs_file_close(&m_lfs, &file);

        return MA_EIO;
    }

    ret = lfs_file_close(&m_lfs, &file);
    if (ret != LFS_ERR_OK) [[unlikely]] {
        MA_LOGE(TAG, "Failed to close file %s", path.c_str());
        return MA_EIO;
    }

    return MA_OK;
}

ma_err_t StorageLfs::getImpl(std::string key, std::string& buffer) {
    if (!m_initialized) [[unlikely]] {
        return MA_EPERM;
    }

    static_assert(LFS_NAME_MAX > 16);
    auto parts = keyToPath(key, 16);
    if (parts.empty()) [[unlikely]] {
        return MA_EINVAL;
    }

    std::string path;
    for (auto& part : parts) {
        path.push_back('/');
        path += std::move(part);
    }
    parts.clear();

    lfs_file_t file;
    int        ret = lfs_file_open(&m_lfs, &file, path.c_str(), LFS_O_RDONLY);
    if (ret != LFS_ERR_OK) [[unlikely]] {
        MA_LOGE(TAG, "Failed to open file %s, code %d", path.c_str(), ret);
        return MA_EIO;
    }

    lfs_info info;
    ret = lfs_stat(&m_lfs, path.c_str(), &info);
    if (ret != LFS_ERR_OK) [[unlikely]] {
        MA_LOGE(TAG, "Failed to stat file %s", path.c_str());
        return MA_EIO;
    }
    const size_t size = info.size;
    buffer.resize(size);
    lfs_size_t read = lfs_file_read(&m_lfs, &file, buffer.data(), size);
    if (read != size) [[unlikely]] {
        MA_LOGE(TAG, "Failed to read from file %s, code %d", path.c_str(), read);

        lfs_file_close(&m_lfs, &file);

        return MA_EIO;
    }

    ret = lfs_file_close(&m_lfs, &file);
    if (ret != LFS_ERR_OK) [[unlikely]] {
        MA_LOGE(TAG, "Failed to close file %s", path.c_str());
        return MA_EIO;
    }

    return MA_OK;
}

ma_err_t StorageLfs::remove(const std::string& key) {
    Guard guard(m_mutex);

    if (!m_initialized) [[unlikely]] {
        return MA_EPERM;
    }

    static_assert(LFS_NAME_MAX > 16);
    auto parts = keyToPath(key, 16);
    if (parts.empty()) [[unlikely]] {
        return MA_EINVAL;
    }

    std::string path;
    for (const auto& part : parts) {
        path.push_back('/');
        path += part;
    }

    for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
        int ret = lfs_remove(&m_lfs, path.c_str());
        if (ret == LFS_ERR_NOENT || ret == LFS_ERR_NOTEMPTY) {
            break;
        } else if (ret != LFS_ERR_OK) [[unlikely]] {
            MA_LOGE(TAG, "Failed to remove file %s", path.c_str());
            return MA_EIO;
        }

        path = path.substr(0, path.size() - it->size());
        path.pop_back();
    }

    return MA_OK;
}

bool StorageLfs::exists(const std::string& key) {
    Guard guard(m_mutex);

    if (!m_initialized) [[unlikely]] {
        return false;
    }

    static_assert(LFS_NAME_MAX > 16);
    auto parts = keyToPath(key, 16);
    if (parts.empty()) [[unlikely]] {
        return false;
    }

    std::string path;
    for (const auto& part : parts) {
        path.push_back('/');
        path += part;
    }

    lfs_info info;
    int      ret = lfs_stat(&m_lfs, path.c_str(), &info);
    if (ret == LFS_ERR_OK) {
        return true;
    }

    return false;
}

ma_err_t StorageLfs::set(const std::string& key, const void* value, size_t size) {
    Guard guard(m_mutex);
    return setImpl(key, value, size);
}

ma_err_t StorageLfs::get(const std::string& key, std::string& value) {
    Guard guard(m_mutex);
    return getImpl(key, value);
}

ma_err_t StorageLfs::set(const std::string& key, int64_t value) {
    Guard guard(m_mutex);
    return setImpl(key, &value, sizeof(value));
}

ma_err_t StorageLfs::set(const std::string& key, double value) {
    Guard guard(m_mutex);
    return setImpl(key, &value, sizeof(value));
}

ma_err_t StorageLfs::get(const std::string& key, int64_t& value) {
    Guard guard(m_mutex);

    std::string buffer;
    ma_err_t    err = getImpl(key, buffer);
    if (err != MA_OK) {
        return err;
    }

    if (buffer.size() != sizeof(value)) {
        return MA_EINVAL;
    }

    std::memcpy(&value, buffer.data(), sizeof(value));

    return MA_OK;
}

ma_err_t StorageLfs::get(const std::string& key, double& value) {
    Guard guard(m_mutex);

    std::string buffer;
    ma_err_t    err = getImpl(key, buffer);
    if (err != MA_OK) {
        return err;
    }

    if (buffer.size() != sizeof(value)) {
        return MA_EINVAL;
    }

    std::memcpy(&value, buffer.data(), sizeof(value));

    return MA_OK;
}

}  // namespace ma

#endif
