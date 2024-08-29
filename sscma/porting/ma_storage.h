#ifndef _MA_STORAGE_H_
#define _MA_STORAGE_H_

#include <cstring>
#include <string>

#include "core/ma_common.h"

#ifndef MA_STORAGE_CHECK_NULL_PTR
    #define MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage) \
        if (!p_storage) {                              \
            break;                                     \
        }
#endif

#if defined(MA_STORAGE_SET_POD)
    #undef MA_STORAGE_SET_POD
#endif
#define MA_STORAGE_SET_POD(ret, p_storage, key, value)    \
    do {                                                  \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)        \
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
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)                    \
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

#if defined(MA_STORAGE_GET_STR)
    #undef MA_STORAGE_GET_STR
#endif
#define MA_STORAGE_GET_STR(p_storage, key, value, _default) \
    do {                                                    \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)          \
        std::string buffer;                                 \
        auto        ret = p_storage->get(key, buffer);      \
        if (ret != MA_OK) {                                 \
            value = _default;                               \
            break;                                          \
        }                                                   \
        value = std::move(buffer);                          \
    } while (0)

#if defined(MA_STORAGE_GET_CSTR)
    #undef MA_STORAGE_GET_CSTR
#endif
#define MA_STORAGE_GET_CSTR(p_storage, key, value, size, _default) \
    do {                                                           \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)                 \
        std::string buffer;                                        \
        auto        ret = p_storage->get(key, buffer);             \
        if (ret != MA_OK) {                                        \
            std::strncpy(value, _default, size);                   \
        }                                                          \
        std::strncpy(value, buffer.c_str(), size);                 \
    } while (0)

#if defined(MA_STORAGE_GET_ASTR)
    #undef MA_STORAGE_GET_ASTR
#endif
#define MA_STORAGE_GET_ASTR(p_storage, key, value, _default) \
    MA_STORAGE_GET_CSTR(p_storage, key, value, sizeof(value) - 1, _default)

#if defined(MA_STORAGE_GET_POD)
    #undef MA_STORAGE_GET_POD
#endif
#define MA_STORAGE_GET_POD(p_storage, key, value, _default)         \
    do {                                                            \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)                  \
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