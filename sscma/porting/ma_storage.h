#ifndef _MA_STORAGE_H_
#define _MA_STORAGE_H_

#include <ma_config_board.h>

#include <cstring>
#include <string>

#define MA_STORAGE_ENABLE

#if not defined(MA_STORAGE_ENABLE)
#define MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)                 ((void)0)
#define MA_STORAGE_SET_STR(ret, p_storage, key, value)             ((void)0)
#define MA_STORAGE_NOSTA_SET_STR(p_storage, key, value)            ((void)0)
#define MA_STORAGE_SET_CSTR(ret, p_storage, key, value)            ((void)0)
#define MA_STORAGE_NOSTA_SET_CSTR(p_storage, key, value)           ((void)0)
#define MA_STORAGE_SET_ASTR(ret, p_storage, key, value)            ((void)0)
#define MA_STORAGE_NOSTA_SET_ASTR(p_storage, key, value)           ((void)0)
#define MA_STORAGE_SET_POD(ret, p_storage, key, value)             ((void)0)
#define MA_STORAGE_NOSTA_SET_POD(p_storage, key, value)            ((void)0)
#define MA_STORAGE_SET_RVPOD(ret, p_storage, key, value)           ((void)0)
#define MA_STORAGE_NOSTA_SET_RVPOD(p_storage, key, value)          ((void)0)
#define MA_STORAGE_GET_STR(p_storage, key, value, _default)        ((void)0)
#define MA_STORAGE_GET_CSTR(p_storage, key, value, size, _default) ((void)0)
#define MA_STORAGE_GET_ASTR(p_storage, key, value, _default)       ((void)0)
#define MA_STORAGE_GET_POD(p_storage, key, value, _default)        ((void)0)
#define MA_STORAGE_REMOVE(ret, p_storage, key)                     ((void)0)
#define MA_STORAGE_NOSTA_REMOVE(p_storage, key)                    ((void)0)
#define MA_STORAGE_EXISTS(p_storage, key)                          ((void)0)
#endif

#if not defined(MA_STORAGE_CHECK_NULL_PTR)
#define MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage) \
    if (!p_storage) {                              \
        break;                                     \
    }
#endif

#if not defined(MA_STORAGE_SET_STR)
#define MA_STORAGE_SET_STR(ret, p_storage, key, value)         \
    do {                                                       \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)             \
        ret = p_storage->set(key, value.data(), value.size()); \
    } while (0)
#endif

#if not defined(MA_STORAGE_NOSTA_SET_STR)
#define MA_STORAGE_NOSTA_SET_STR(p_storage, key, value) \
    do {                                                \
        [[maybe_unused]] int ret;                       \
        MA_STORAGE_SET_STR(ret, p_storage, key, value); \
    } while (0)
#endif

#if not defined(MA_STORAGE_SET_CSTR)
#define MA_STORAGE_SET_CSTR(ret, p_storage, key, value)       \
    do {                                                      \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)            \
        ret = p_storage->set(key, value, std::strlen(value)); \
    } while (0)
#endif

#if not defined(MA_STORAGE_NOSTA_SET_CSTR)
#define MA_STORAGE_NOSTA_SET_CSTR(p_storage, key, value) \
    do {                                                 \
        [[maybe_unused]] int ret;                        \
        MA_STORAGE_SET_CSTR(ret, p_storage, key, value); \
    } while (0)
#endif

#if not defined(MA_STORAGE_SET_ASTR)
#define MA_STORAGE_SET_ASTR(ret, p_storage, key, value)      \
    do {                                                     \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)           \
        ret = p_storage->set(key, value, sizeof(value) - 1); \
    } while (0)
#endif

#if not defined(MA_STORAGE_NOSTA_SET_ASTR)
#define MA_STORAGE_NOSTA_SET_ASTR(p_storage, key, value) \
    do {                                                 \
        [[maybe_unused]] int ret;                        \
        MA_STORAGE_SET_ASTR(ret, p_storage, key, value); \
    } while (0)
#endif

#if not defined(MA_STORAGE_SET_POD)
#define MA_STORAGE_SET_POD(ret, p_storage, key, value)                                                                                                                                                \
    do {                                                                                                                                                                                              \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)                                                                                                                                                    \
        if constexpr (std::is_same_v<decltype(value), int8_t> || std::is_same_v<decltype(value), uint8_t> || std::is_same_v<decltype(value), int16_t> || std::is_same_v<decltype(value), uint16_t> || \
                      std::is_same_v<decltype(value), uint32_t> || std::is_same_v<decltype(value), int32_t> || std::is_same_v<decltype(value), int64_t> ||                                            \
                      std::is_same_v<decltype(value), uint64_t>) {                                                                                                                                    \
            ret = p_storage->set(key, static_cast<int64_t>(value));                                                                                                                                   \
        } else if constexpr (std::is_same_v<decltype(value), float> || std::is_same_v<decltype(value), double>) {                                                                                     \
            ret = p_storage->set(key, static_cast<double>(value));                                                                                                                                    \
        } else {                                                                                                                                                                                      \
            ret = p_storage->set(key, &value, sizeof(value));                                                                                                                                         \
        }                                                                                                                                                                                             \
    } while (0)
#endif

#if not defined(MA_STORAGE_NOSTA_SET_POD)
#define MA_STORAGE_NOSTA_SET_POD(p_storage, key, value) \
    do {                                                \
        [[maybe_unused]] int ret;                       \
        MA_STORAGE_SET_POD(ret, p_storage, key, value); \
    } while (0)
#endif

#if not defined(MA_STORAGE_SET_RVPOD)
#define MA_STORAGE_SET_RVPOD(ret, p_storage, key, value)                                                                                                                                              \
    do {                                                                                                                                                                                              \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)                                                                                                                                                    \
        if constexpr (std::is_same_v<decltype(value), int8_t> || std::is_same_v<decltype(value), uint8_t> || std::is_same_v<decltype(value), int16_t> || std::is_same_v<decltype(value), uint16_t> || \
                      std::is_same_v<decltype(value), uint32_t> || std::is_same_v<decltype(value), int32_t> || std::is_same_v<decltype(value), int64_t> ||                                            \
                      std::is_same_v<decltype(value), uint64_t>) {                                                                                                                                    \
            ret = p_storage->set(key, static_cast<int64_t>(value));                                                                                                                                   \
        } else if constexpr (std::is_same_v<decltype(value), float> || std::is_same_v<decltype(value), double>) {                                                                                     \
            ret = p_storage->set(key, static_cast<double>(value));                                                                                                                                    \
        } else {                                                                                                                                                                                      \
            decltype(value) tmp = value;                                                                                                                                                              \
            ret                 = p_storage->set(key, &tmp, sizeof(tmp));                                                                                                                             \
        }                                                                                                                                                                                             \
    } while (0)
#endif

#if not defined(MA_STORAGE_NOSTA_SET_RVPOD)
#define MA_STORAGE_NOSTA_SET_RVPOD(p_storage, key, value) \
    do {                                                  \
        [[maybe_unused]] int ret;                         \
        MA_STORAGE_SET_RVPOD(ret, p_storage, key, value); \
    } while (0)
#endif

#if not defined(MA_STORAGE_GET_STR)
#define MA_STORAGE_GET_STR(p_storage, key, value, _default) \
    do {                                                    \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)          \
        std::string buffer;                                 \
        auto ret = p_storage->get(key, buffer);             \
        if (ret != MA_OK) {                                 \
            value = _default;                               \
            break;                                          \
        }                                                   \
        value = std::move(buffer);                          \
    } while (0)
#endif

#if not defined(MA_STORAGE_GET_CSTR)
#define MA_STORAGE_GET_CSTR(p_storage, key, value, size, _default) \
    do {                                                           \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)                 \
        std::string buffer;                                        \
        auto ret = p_storage->get(key, buffer);                    \
        if (ret != MA_OK) {                                        \
            std::strncpy(value, _default, size);                   \
        } else {                                                   \
            std::strncpy(value, buffer.c_str(), size);             \
        }                                                          \
    } while (0)
#endif

#if not defined(MA_STORAGE_GET_ASTR)
#define MA_STORAGE_GET_ASTR(p_storage, key, value, _default) MA_STORAGE_GET_CSTR(p_storage, key, value, sizeof(value) - 1, _default)
#endif

#if not defined(MA_STORAGE_GET_POD)
#define MA_STORAGE_GET_POD(p_storage, key, value, _default)                                                                                                                                           \
    do {                                                                                                                                                                                              \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)                                                                                                                                                    \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage)                                                                                                                                                    \
        if constexpr (std::is_same_v<decltype(value), int8_t> || std::is_same_v<decltype(value), uint8_t> || std::is_same_v<decltype(value), int16_t> || std::is_same_v<decltype(value), uint16_t> || \
                      std::is_same_v<decltype(value), uint32_t> || std::is_same_v<decltype(value), int32_t> || std::is_same_v<decltype(value), int64_t> ||                                            \
                      std::is_same_v<decltype(value), uint64_t>) {                                                                                                                                    \
            int64_t tmp = static_cast<int64_t>(_default);                                                                                                                                             \
            p_storage->get(key, tmp);                                                                                                                                                                 \
            value = static_cast<decltype(value)>(tmp);                                                                                                                                                \
        } else if constexpr (std::is_same_v<decltype(value), float> || std::is_same_v<decltype(value), double>) {                                                                                     \
            double tmp = static_cast<double>(_default);                                                                                                                                               \
            p_storage->get(key, tmp);                                                                                                                                                                 \
            value = static_cast<decltype(value)>(tmp);                                                                                                                                                \
        } else {                                                                                                                                                                                      \
            std::string buffer;                                                                                                                                                                       \
            ma_err_t err = p_storage->get(key, buffer);                                                                                                                                               \
            if (err != MA_OK || sizeof(value) > buffer.size()) {                                                                                                                                      \
                value = _default;                                                                                                                                                                     \
                break;                                                                                                                                                                                \
            }                                                                                                                                                                                         \
            value = *reinterpret_cast<decltype(value)*>(buffer.data());                                                                                                                               \
        }                                                                                                                                                                                             \
    } while (0)
#endif

#if not defined(MA_STORAGE_REMOVE)
#define MA_STORAGE_REMOVE(ret, p_storage, key)     \
    do {                                           \
        MA_STORAGE_CHECK_NULL_PTR_BREAK(p_storage) \
        ret = p_storage->remove(key);              \
    } while (0)
#endif

#if not defined(MA_STORAGE_NOSTA_REMOVE)
#define MA_STORAGE_NOSTA_REMOVE(p_storage, key) \
    do {                                        \
        [[maybe_unused]] int ret;               \
        MA_STORAGE_REMOVE(ret, p_storage, key); \
    } while (0)
#endif

#if not defined(MA_STORAGE_EXISTS)
#if defined(MA_STORAGE_CHECK_NULL_PTR)
#define MA_STORAGE_EXISTS(p_storage, key)                    \
    [](Storage* p_storage, const std::string& str) -> bool { \
        return p_storage && p_storage->exists(str);          \
    }(p_storage, key)
#else
#define MA_STORAGE_EXISTS(p_storage, key)                    \
    [](Storage* p_storage, const std::string& str) -> bool { \
        return p_storage->exists(str);                       \
    }(p_storage, key)
#endif
#endif

namespace ma {

class Storage {
public:
    Storage() : m_initialized(false) {}
    virtual ~Storage() = default;

    Storage(const Storage&)            = delete;
    Storage& operator=(const Storage&) = delete;

    virtual ma_err_t init(const void* config) noexcept = 0;
    virtual void deInit() noexcept                     = 0;

    operator bool() const noexcept {
        return m_initialized;
    }

    virtual ma_err_t set(const std::string& key, int64_t value) noexcept                  = 0;
    virtual ma_err_t set(const std::string& key, double value) noexcept                   = 0;
    virtual ma_err_t get(const std::string& key, int64_t& value) noexcept                 = 0;
    virtual ma_err_t get(const std::string& key, double& value) noexcept                  = 0;
    virtual ma_err_t set(const std::string& key, const void* value, size_t size) noexcept = 0;
    virtual ma_err_t get(const std::string& key, std::string& value) noexcept             = 0;

    virtual ma_err_t remove(const std::string& key) noexcept = 0;
    virtual bool exists(const std::string& key) noexcept     = 0;

protected:
    bool m_initialized;
};

}  // namespace ma

#endif  // _MA_STORAGE_H_