#ifndef _MA_STORAGE_LFS_H_
#define _MA_STORAGE_LFS_H_

#ifdef MA_FILESYSTEM_LITTLEFS

    #include <cstdint>
    #include <list>
    #include <string>

    #include "core/ma_types.h"
    #include "lfs.h"
    #include "ma_config_board.h"
    #include "ma_osal.h"
    #include "ma_storage.h"

    #ifdef MA_STORAGE_LFS_USE_FLASHBD
        #include "bd/lfs_flashbd.h"
        #define ma_lfs_bd_t       lfs_flashbd_t
        #define ma_lfs_bd_cfg_t   lfs_flashbd_config
        #define ma_lfs_bd_destory lfs_flashbd_destroy
    #elif defined(MA_STORAGE_LFS_USE_FILEBD)
        #include "bd/lfs_filebd.h"
        #define ma_lfs_bd_t     lfs_filebd_t
        #define ma_lfs_bd_cfg_t lfs_filebd_config
    #elif defined(MA_STORAGE_LFS_USE_RAMBD)
        #include "bd/lfs_rambd.h"
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

    ma_err_t init(void* config) override;
    ma_err_t deInit() override;

    virtual ma_err_t set(const std::string& key, const void* value, size_t size) override;
    virtual ma_err_t get(const std::string& key, std::string& value) override;

    virtual ma_err_t remove(const std::string& key) override;
    virtual bool     exists(const std::string& key) override;

   private:
    std::list<std::string> keyToPath(const std::string& key, size_t break_size);

    ma_err_t setImpl(std::string key, void const* data, size_t size);
    ma_err_t getImpl(std::string key, std::string& buffer);

   private:
    Mutex m_mutex;

    ma_lfs_bd_t m_bd;
    lfs_t       m_lfs;
    lfs_config  m_fs_config;
};

}  // namespace ma

#endif

#endif
