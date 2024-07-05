#ifndef _MA_STORAGE_FILE_H_
#define _MA_STORAGE_FILE_H_

#include <cJSON.h>
#include <fstream>

#include "core/ma_common.h"

#include "porting/ma_osal.h"
#include "porting/ma_storage.h"

namespace ma {

class StorageFile : public Storage {
public:
    StorageFile();
    StorageFile(const std::string& filename);
    virtual ~StorageFile();

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
    virtual ma_err_t get(const std::string& key,
                         std::string& value,
                         const std::string default_ = "") override;
    virtual ma_err_t get(const std::string& key, char* value, const char* default_ = "") override;

    virtual ma_err_t remove(const std::string& key) override;
    virtual bool exists(const std::string& key) override;

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
