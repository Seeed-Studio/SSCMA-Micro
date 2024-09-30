#ifndef _MA_STORAGE_FILE_H_
#define _MA_STORAGE_FILE_H_

#include <fstream>

#include <cJSON.h>

#include "core/ma_common.h"

#include "porting/ma_osal.h"
#include "porting/ma_storage.h"

namespace ma {

class StorageFile : public Storage {
public:
    StorageFile();
    ~StorageFile();

    ma_err_t init(const void* config) noexcept override;
    void deInit() noexcept override;


    ma_err_t set(const std::string& key, int64_t value) noexcept override;
    ma_err_t set(const std::string& key, double value) noexcept override;
    ma_err_t get(const std::string& key, int64_t& value) noexcept override;
    ma_err_t get(const std::string& key, double& value) noexcept override;

    ma_err_t set(const std::string& key, const void* value, size_t size) noexcept override;
    ma_err_t get(const std::string& key, std::string& value) noexcept override;

    ma_err_t remove(const std::string& key) noexcept override;
    bool exists(const std::string& key) noexcept override;

private:
    std::string filename_;
    cJSON* root_;
    Mutex mutex_;

    void save();
    void load();
    cJSON* getNode(const std::string& key);
    ma_err_t setNode(const std::string& key, cJSON* value);
};

}  // namespace ma

#endif  // _JSON_STORAGE_H_
