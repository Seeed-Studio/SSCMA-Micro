#ifndef _MA_STORAGE_FILE_H_
#define _MA_STORAGE_FILE_H_

#include "porting/ma_storage.h"
#include <string>

namespace ma {

class StorageFile : public Storage {
public:
    StorageFile(const std::string& filename);
    ~StorageFile() override;

    ma_err_t set(const std::string& key, bool value) override;
    ma_err_t set(const std::string& key, int8_t value) override;
    ma_err_t set(const std::string& key, int16_t value) override;
    ma_err_t set(const std::string& key, int32_t value) override;
    ma_err_t set(const std::string& key, int64_t value) override;
    ma_err_t set(const std::string& key, uint8_t value) override;
    ma_err_t set(const std::string& key, uint16_t value) override;
    ma_err_t set(const std::string& key, uint32_t value) override;
    ma_err_t set(const std::string& key, uint64_t value) override;
    ma_err_t set(const std::string& key, float value) override;
    ma_err_t set(const std::string& key, double value) override;
    ma_err_t set(const std::string& key, const std::string& value) override;
    ma_err_t set(const std::string& key, void* value, size_t size) override;

    ma_err_t get(const std::string& key, bool& value) override;
    ma_err_t get(const std::string& key, int8_t& value) override;
    ma_err_t get(const std::string& key, int16_t& value) override;
    ma_err_t get(const std::string& key, int32_t& value) override;
    ma_err_t get(const std::string& key, int64_t& value) override;
    ma_err_t get(const std::string& key, uint8_t& value) override;
    ma_err_t get(const std::string& key, uint16_t& value) override;
    ma_err_t get(const std::string& key, uint32_t& value) override;
    ma_err_t get(const std::string& key, uint64_t& value) override;
    ma_err_t get(const std::string& key, float& value) override;
    ma_err_t get(const std::string& key, double& value) override;
    ma_err_t get(const std::string& key, std::string& value) override;

    ma_err_t remove(const std::string& key) override;

private:
    std::string filename_;

    ma_err_t set_impl(const std::string& key, const std::string& value);
    ma_err_t get_impl(const std::string& key, std::string& value);
};

}  // namespace ma

#endif  // _MA_STORAGE_FILE_H_
