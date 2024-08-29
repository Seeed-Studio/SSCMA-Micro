#ifndef _MA_STORAGE_H_
#define _MA_STORAGE_H_

#include <cstring>
#include <string>

#include "core/ma_common.h"

#if defined(MA_STORAGE_SET_POD)
    #undef MA_STORAGE_SET_POD
#endif
#define MA_STORAGE_SET_POD(ret, p_storage, key, value)    \
    do {                                                  \
        if (!p_storage) {                                 \
            break;                                        \
        }                                                 \
        ret = p_storage->set(key, &value, sizeof(value)); \
    } while (0)

#if defined(MA_STORAGE_NOSTA_SET_POD)
    #undef MA_STORAGE_NOSTA_SET_POD
#endif
#define MA_STORAGE_NOSTA_SET_POD(p_storage, key, value) \
    do {                                                \
        [[maybe_unused]] int ret;                       \
        MA_STORAGE_SET_POD(ret, p_storage, key, value); \
    } while (0)

#if defined(MA_STORAGE_SET_RVPOD)
    #undef MA_STORAGE_SET_RVPOD
#endif
#define MA_STORAGE_SET_RVPOD(ret, p_storage, key, value)              \
    do {                                                              \
        if (!p_storage) {                                             \
            break;                                                    \
        }                                                             \
        decltype(value) tmp = value;                                  \
        ret                 = p_storage->set(key, &tmp, sizeof(tmp)); \
    } while (0)

#if defined(MA_STORAGE_NOSTA_SET_RVPOD)
    #undef MA_STORAGE_NOSTA_SET_RVPOD
#endif
#define MA_STORAGE_NOSTA_SET_RVPOD(p_storage, key, value) \
    do {                                                  \
        [[maybe_unused]] int ret;                         \
        MA_STORAGE_SET_RVPOD(ret, p_storage, key, value); \
    } while (0)

#if defined(MA_STORAGE_GET_CSTR)
    #undef MA_STORAGE_GET_CSTR
#endif
#define MA_STORAGE_GET_CSTR(p_storage, key, value, size, _default) \
    do {                                                           \
        if (!p_storage) {                                          \
            break;                                                 \
        }                                                          \
        std::string buffer;                                        \
        auto        ret = p_storage->get(key, buffer);             \
        if (ret != MA_OK) {                                        \
            std::strncpy(value, _default, size);                   \
        }                                                          \
        std::strncpy(value, buffer.c_str(), size);                 \
    } while (0)

#if defined(MA_STORAGE_GET_POD)
    #undef MA_STORAGE_GET_POD
#endif
#define MA_STORAGE_GET_POD(p_storage, key, value, _default)         \
    do {                                                            \
        if (!p_storage) {                                           \
            break;                                                  \
        }                                                           \
        std::string buffer;                                         \
        ma_err_t    err = p_storage->get(key, buffer);              \
        if (err != MA_OK) {                                         \
            value = _default;                                       \
            break;                                                  \
        }                                                           \
        value = *reinterpret_cast<decltype(value)*>(buffer.data()); \
    } while (0)

namespace ma {

class Storage {
   public:
    Storage()          = default;
    virtual ~Storage() = default;

    virtual ma_err_t set(const std::string& key, const std::string& value)       = 0;
    virtual ma_err_t set(const std::string& key, const void* value, size_t size) = 0;

    virtual ma_err_t get(const std::string& key, std::string& value) = 0;

    virtual ma_err_t remove(const std::string& key) = 0;
    virtual bool     exists(const std::string& key) = 0;
};

}  // namespace ma

#endif  // _MA_STORAGE_H_