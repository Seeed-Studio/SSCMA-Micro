#ifndef _MA_STORAGE_H_
#define _MA_STORAGE_H_

#include "core/ma_common.h"

#include <string>

namespace ma {

class Storage {
public:
    Storage()          = default;
    virtual ~Storage() = default;

    // virtual ma_err_t set(const std::string& key, bool value)               = 0;
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
    // virtual ma_err_t get(const std::string& key, bool& value)              = 0;
    virtual ma_err_t get(const std::string& key, int8_t& value, int8_t default_ = 0)            = 0;
    virtual ma_err_t get(const std::string& key, int16_t& value, int16_t default_ = 0)          = 0;
    virtual ma_err_t get(const std::string& key, int32_t& value, int32_t default_ = 0)          = 0;
    virtual ma_err_t get(const std::string& key, int64_t& value, int64_t default_ = 0)          = 0;
    virtual ma_err_t get(const std::string& key, uint8_t& value, uint8_t default_ = 0)          = 0;
    virtual ma_err_t get(const std::string& key, uint16_t& value, uint16_t default_ = 0)        = 0;
    virtual ma_err_t get(const std::string& key, uint32_t& value, uint32_t default_ = 0)        = 0;
    virtual ma_err_t get(const std::string& key, uint64_t& value, uint64_t default_ = 0)        = 0;
    virtual ma_err_t get(const std::string& key, float& value, float default_ = 0)              = 0;
    virtual ma_err_t get(const std::string& key, double& value, double default_ = 0)            = 0;
    virtual ma_err_t get(const std::string& key, std::string& value, const std::string default_ = "") = 0;
    virtual ma_err_t get(const std::string& key, char* value, const char* default_ = "")              = 0;

    virtual ma_err_t remove(const std::string& key) = 0;
    virtual bool exists(const std::string& key)     = 0;
};
}  // namespace ma

#endif  // _MA_STORAGE_H_