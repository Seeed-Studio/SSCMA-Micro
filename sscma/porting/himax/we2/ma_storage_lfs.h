#ifndef _MA_STORAGE_LFS_H_
#define _MA_STORAGE_LFS_H_

#ifdef MA_FILESYSTEM_LITTLEFS

    #include <core/ma_types.h>
    #include <lfs.h>
    #include <porting/ma_osal.h>
    #include <porting/ma_storage.h>

    #include <cstdint>
    #include <list>
    #include <string>

    #include "ma_config_board.h"

    #ifdef MA_STORAGE_LFS_USE_FLASHBD
        #include <bd/lfs_flashbd.h>
        #define ma_lfs_bd_t       lfs_flashbd_t
        #define ma_lfs_bd_cfg_t   lfs_flashbd_config
        #define ma_lfs_bd_destory lfs_flashbd_destroy
    #elif defined(MA_STORAGE_LFS_USE_FILEBD)
        #include <bd/lfs_filebd.h>
        #define ma_lfs_bd_t     lfs_filebd_t
        #define ma_lfs_bd_cfg_t lfs_filebd_config
    #elif defined(MA_STORAGE_LFS_USE_RAMBD)
        #include <bd/lfs_rambd.h>
        #define ma_lfs_bd_t     lfs_rambd_t
        #define ma_lfs_bd_cfg_t lfs_rambd_config
    #else
        #error "No block device specified"
    #endif

namespace ma {

class StorageLfs final : public Storage {
   public:
    StorageLfs();
    virtual ~StorageLfs();

    ma_err_t init(const void* config) noexcept override;
    void     deInit() noexcept override;

    virtual ma_err_t set(const std::string& key, const void* value, size_t size) noexcept override;
    virtual ma_err_t get(const std::string& key, std::string& value) noexcept override;

    virtual ma_err_t remove(const std::string& key) noexcept override;
    virtual bool     exists(const std::string& key) noexcept override;

    virtual ma_err_t set(const std::string& key, int64_t value) noexcept override;
    virtual ma_err_t set(const std::string& key, double value) noexcept override;
    virtual ma_err_t get(const std::string& key, int64_t& value) noexcept override;
    virtual ma_err_t get(const std::string& key, double& value) noexcept override;

   private:
    std::list<std::string> keyToPath(const std::string& key, size_t break_size) noexcept;

    ma_err_t setImpl(std::string key, void const* data, size_t size) noexcept;
    ma_err_t getImpl(std::string key, std::string& buffer) noexcept;

   private:
    Mutex m_mutex;

    ma_lfs_bd_t m_bd;
    lfs_t       m_lfs;
    lfs_config  m_fs_config;
};

}  // namespace ma

#endif

#endif
