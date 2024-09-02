#ifndef _MA_STORAGE_LFS_H_
#define _MA_STORAGE_LFS_H_

#include <cstdint>
#include <list>
#include <string>

#include "bd/lfs_flashbd.h"
#include "core/ma_common.h"
#include "core/ma_types.h"
#include "lfs.h"
#include "porting/ma_osal.h"
#include "porting/ma_storage.h"

namespace ma {

class StorageLfs : public Storage {
   public:
    StorageLfs(size_t bd_size);
    virtual ~StorageLfs();

    ma_err_t mount(bool force = true);
    ma_err_t uMount();

    virtual ma_err_t set(const std::string& key, const void* value, size_t size) override;

    virtual ma_err_t get(const std::string& key, std::string& value) override;

    virtual ma_err_t remove(const std::string& key) override;
    virtual bool     exists(const std::string& key) override;

   protected:
    StorageLfs();

    std::list<std::string> keyToPath(const std::string& key, size_t break_size);

    ma_err_t setImpl(std::string key, void const* data, size_t size);
    ma_err_t getImpl(std::string key, std::string& buffer);

   private:
    bool  is_mounted_;
    Mutex mutex_;

    size_t      bd_size_;

    lfs_flashbd_t bd_;
    lfs_t         lfs_;

    lfs_flashbd_config bd_config_;
    lfs_config         fs_config_;
};

}  // namespace ma

#endif
