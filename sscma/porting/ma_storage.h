#ifndef _MA_STORAGE_H_
#define _MA_STORAGE_H_

#include "core/ma_common.h"
#include <string>

namespace ma {

class Storage {
public:
    Storage()  = default;
    virtual ~Storage() = default;


    virtual ma_err_t set(const std::string& key, bool value)               = 0;
    virtual ma_err_t set(const std::string& key, int8_t value)             = 0;
    virtual ma_err_t set(const std::string& key, int16_t value)            = 0;
    virtual ma_err_t set(const std::string& key, int32_t value)            = 0;
    virtual ma_err_t set(const std::string& key, int64_t value)            = 0;
    virtual ma_err_t set(const std::string& key, uint8_t value)            = 0;
    virtual ma_err_t set(const std::string& key, uint16_t value)           = 0;
    virtual ma_err_t set(const std::string& key, uint32_t value)           = 0;
    virtual ma_err_t set(const std::string& key, uint64_t value)           = 0;
    virtual ma_err_t set(const std::string& key, float value)              = 0;
    virtual ma_err_t set(const std::string& key, double value)             = 0;
    virtual ma_err_t set(const std::string& key, const std::string& value) = 0;
    virtual ma_err_t set(const std::string& key, void* value, size_t size) = 0;
    virtual ma_err_t get(const std::string& key, bool& value)              = 0;
    virtual ma_err_t get(const std::string& key, int8_t& value)            = 0;
    virtual ma_err_t get(const std::string& key, int16_t& value)           = 0;
    virtual ma_err_t get(const std::string& key, int32_t& value)           = 0;
    virtual ma_err_t get(const std::string& key, int64_t& value)           = 0;
    virtual ma_err_t get(const std::string& key, uint8_t& value)           = 0;
    virtual ma_err_t get(const std::string& key, uint16_t& value)          = 0;
    virtual ma_err_t get(const std::string& key, uint32_t& value)          = 0;
    virtual ma_err_t get(const std::string& key, uint64_t& value)          = 0;
    virtual ma_err_t get(const std::string& key, float& value)             = 0;
    virtual ma_err_t get(const std::string& key, double& value)            = 0;
    virtual ma_err_t get(const std::string& key, std::string& value)       = 0;

    virtual ma_err_t remove(const std::string& key) = 0;
};
}  // namespace ma

#endif  // _MA_STORAGE_H_