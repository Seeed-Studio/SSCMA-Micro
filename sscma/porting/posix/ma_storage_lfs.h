#ifndef _MA_STORAGE_LFS_H_
#define _MA_STORAGE_LFS_H_

#include <cstdint>
#include <string>

#include "bd/lfs_filebd.h"
#include "core/ma_common.h"
#include "core/ma_types.h"
#include "lfs.h"
#include "porting/ma_osal.h"
#include "porting/ma_storage.h"

namespace ma {

class StorageLfs : public Storage {
   public:
    StorageLfs();
    StorageLfs(const std::string& mount_point, size_t bd_size = 8 * 1024 * 1024);
    virtual ~StorageLfs();

    ma_err_t mount();
    ma_err_t uMount();

    // virtual ma_err_t set(const std::string& key, bool value) override;
    virtual ma_err_t set(const std::string& key, int8_t value) override;
    virtual ma_err_t set(const std::string& key, int16_t value) override;
    virtual ma_err_t set(const std::string& key, int32_t value) override;
    virtual ma_err_t set(const std::string& key, int64_t value) override;
    virtual ma_err_t set(const std::string& key, uint8_t value) override;
    virtual ma_err_t set(const std::string& key, uint16_t value) override;
    virtual ma_err_t set(const std::string& key, uint32_t value) override;
    virtual ma_err_t set(const std::string& key, uint64_t value) override;
    virtual ma_err_t set(const std::string& key, float value) override;
    virtual ma_err_t set(const std::string& key, double value) override;
    virtual ma_err_t set(const std::string& key, const std::string& value) override;
    virtual ma_err_t set(const std::string& key, void* value, size_t size) override;

    // virtual ma_err_t get(const std::string& key, bool& value) override;
    virtual ma_err_t get(const std::string& key, int8_t& value, int8_t default_ = 0) override;
    virtual ma_err_t get(const std::string& key, int16_t& value, int16_t default_ = 0) override;
    virtual ma_err_t get(const std::string& key, int32_t& value, int32_t default_ = 0) override;
    virtual ma_err_t get(const std::string& key, int64_t& value, int64_t default_ = 0) override;
    virtual ma_err_t get(const std::string& key, uint8_t& value, uint8_t default_ = 0) override;
    virtual ma_err_t get(const std::string& key, uint16_t& value, uint16_t default_ = 0) override;
    virtual ma_err_t get(const std::string& key, uint32_t& value, uint32_t default_ = 0) override;
    virtual ma_err_t get(const std::string& key, uint64_t& value, uint64_t default_ = 0) override;
    virtual ma_err_t get(const std::string& key, float& value, float default_ = 0) override;
    virtual ma_err_t get(const std::string& key, double& value, double default_ = 0) override;
    virtual ma_err_t get(const std::string& key, std::string& value, const std::string default_ = "") override;
    virtual ma_err_t get(const std::string& key, char* value, const char* default_ = "") override;

    virtual ma_err_t remove(const std::string& key) override;
    virtual bool     exists(const std::string& key) override;

   protected:
    ma_err_t setImpl(std::string key, void* data, size_t size);
    ma_err_t getImpl(std::string key, std::string& buffer);

   private:
    bool  is_mounted_;
    Mutex mutex_;

    std::string mount_point_;
    size_t      bd_size_;

    lfs_filebd_t bd_;
    lfs_t        lfs_;

    lfs_filebd_config bd_config_;
    lfs_config        fs_config_;
};

}  // namespace ma

#endif
