/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 nullptr (Seeed Technology Inc.)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _EL_DATA_STORAGE_HPP_
#define _EL_DATA_STORAGE_HPP_

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <forward_list>
#include <iterator>
#include <type_traits>
#include <utility>

#include "core/el_config_internal.h"
#include "core/el_debug.h"
#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"

#define CONFIG_EL_LIB_FLASHDB

#ifdef CONFIG_EL_LIB_FLASHDB

    #include "third_party/FlashDB/flashdb.h"

namespace edgelab {

namespace types {

template <typename ValueType> struct el_storage_kv_t {
    explicit el_storage_kv_t(const char* key_, ValueType&& value_, size_t size_ = sizeof(ValueType)) noexcept
        : key(key_), value(value_), size(size_) {
        EL_ASSERT(size > 0);
    }

    ~el_storage_kv_t() = default;

    const char* key;
    ValueType   value;
    size_t      size;
};

}  // namespace types

using namespace edgelab::types;

namespace utility {

template <typename ValueType,
          typename ValueTypeNoRef = typename std::remove_reference<ValueType>::type,
          typename ValueTypeNoCV  = typename std::remove_cv<ValueType>::type,
          typename std::enable_if<std::is_trivial<ValueTypeNoRef>::value ||
                                  std::is_standard_layout<ValueTypeNoRef>::value>::type* = nullptr>
static inline constexpr el_storage_kv_t<ValueTypeNoCV> el_make_storage_kv(const char* key, ValueType&& value) {
    return el_storage_kv_t<ValueTypeNoCV>(key, std::forward<ValueTypeNoCV>(value));
}

using namespace edgelab::utility;

static inline unsigned long djb2_hash(const unsigned char* bytes) {
    unsigned long hash = 0x1505;
    unsigned char byte;
    while ((byte = *bytes++)) hash = ((hash << 5) + hash) + byte;
    return hash;
}

template <typename T> static inline constexpr const char* get_type_name() { return __PRETTY_FUNCTION__; }

// currently we're not going to manage the life time of the key chars, and std::type_info is unavailable (due to -fno-rtti)
//      1. you can free the buffer using delete[] if you don't need to use the same key anymore (if not, program will crash)
//      2. personally I don't think it a good idea to let el_storage_kv_t to manage the lifetime of the key:
//          a. too compilcate to use const difference to determine whether to delete the key (need to set to nullptr while copy/assign, etc.)
//          b. not wise to use a bool flag to dertermine whether to delete the key (because copy/assign would be a continuation of the key)
template <typename VarType, typename ValueTypeNoCV = typename std::remove_cv<VarType>::type>
static inline constexpr el_storage_kv_t<ValueTypeNoCV> el_make_storage_kv_from_type(VarType&& data) {
    using VarTypeNoCVRef               = typename std::remove_cv<typename std::remove_reference<VarType>::type>::type;
    const char*          type_name     = get_type_name<VarTypeNoCVRef>();
    unsigned long        hash          = djb2_hash(reinterpret_cast<const unsigned char*>(type_name));
    char*                static_buffer = nullptr;
    static const char*   format_str    = "edgelab#type_name#%.8lX";
    static const uint8_t buffer_size   = [&]() -> uint8_t {
        // 1 for terminator, -4 for formatter, 8 for hash hex (unsigned long), -1 for bit hacks (remove last 1 in binary)
        uint8_t len = static_cast<uint8_t>(std::strlen(format_str) + 1u - 4u + 8u - 1u);
        len |= len >> 1;
        len |= len >> 2;
        len |= len >> 4;
        return ++len;  // the nearest power of 2 value to key length
    }();
    static std::forward_list<std::pair<unsigned long, char*>> hash_list{};
    auto it{std::find_if(hash_list.begin(), hash_list.end(), [&](const std::pair<unsigned long, char*>& pair) {
        return pair.first == hash;
    })};
    if (it != hash_list.end())
        static_buffer = it->second;
    else {
        static_buffer = new char[buffer_size]{};  // we're not going to delete it
        std::sprintf(static_buffer, format_str, hash);
        hash_list.emplace_front(hash, static_buffer);
    }
    EL_ASSERT(static_buffer != nullptr);

    return el_make_storage_kv(static_buffer, std::forward<VarType>(data));
}

}  // namespace utility

class Storage {
   public:
    // currently the consistent of Storage is only ensured on a single instance if there're multiple instances that has same name and save path
    Storage();
    ~Storage();

    Storage(const Storage&)            = delete;
    Storage& operator=(const Storage&) = delete;

    el_err_code_t init(const char* name = CONFIG_EL_STORAGE_NAME, const char* path = CONFIG_EL_STORAGE_PATH);
    void          deinit();

    struct Iterator;

    bool contains(const char* key) const;

    // size of the buffer should be equal to handler->value_len
    template <typename ValueType, typename std::enable_if<!std::is_const<ValueType>::value>::type* = nullptr>
    bool get(types::el_storage_kv_t<ValueType>& kv) const {
        const Guard<Mutex> guard(__lock);
        if (!kv.size || !__kvdb) [[unlikely]]
            return false;

        fdb_kv   handler{};
        fdb_kv_t p_handler = fdb_kv_get_obj(__kvdb, kv.key, &handler);
        if (!p_handler || !p_handler->value_len) [[unlikely]]
            return false;

        unsigned char* buffer = new unsigned char[p_handler->value_len];
        fdb_blob       blob{};
        fdb_blob_read(reinterpret_cast<fdb_db_t>(__kvdb),
                      fdb_kv_to_blob(p_handler, fdb_blob_make(&blob, buffer, p_handler->value_len)));
        std::memcpy(&kv.value, buffer, std::min(kv.size, static_cast<size_t>(p_handler->value_len)));
        delete[] buffer;

        return true;
    }

    template <typename ValueType,
              typename std::enable_if<!std::is_const<ValueType>::value &&
                                      std::is_lvalue_reference<ValueType>::value>::type* = nullptr>
    bool get(types::el_storage_kv_t<ValueType>&& kv) const {
        return get(static_cast<types::el_storage_kv_t<ValueType>&>(kv));
    }

    size_t get_value_size(const char* key) const;

    template <
      typename ValueType,
      typename ValueTypeNoCVRef = typename std::remove_cv<typename std::remove_reference<ValueType>::type>::type,
      typename std::enable_if<std::is_trivial<ValueTypeNoCVRef>::value ||
                              std::is_standard_layout<ValueTypeNoCVRef>::value>::type* = nullptr>
    ValueTypeNoCVRef get_value(const char* key) const {
        ValueTypeNoCVRef value{};
        if (!key) [[unlikely]]
            return value;
        auto kv                            = utility::el_make_storage_kv(key, value);
        bool is_ok __attribute__((unused)) = get(kv);
        EL_ASSERT(is_ok);

        return value;
    }

    template <typename KVType> Storage& operator>>(KVType&& kv) {
        bool is_ok __attribute__((unused)) = get(std::forward<KVType>(kv));
        EL_ASSERT(is_ok);

        return *this;
    }

    template <typename ValueType> bool emplace(const types::el_storage_kv_t<ValueType>& kv) {
        const Guard<Mutex> guard(__lock);
        fdb_blob           blob{};
        return fdb_kv_set_blob(__kvdb, kv.key, fdb_blob_make(&blob, &kv.value, kv.size)) == FDB_NO_ERR;
    }

    template <typename KVType> Storage& operator<<(KVType&& kv) {
        bool is_ok __attribute__((unused)) = emplace(std::forward<KVType>(kv));
        EL_ASSERT(is_ok);

        return *this;
    }

    template <typename ValueType> bool try_emplace(const types::el_storage_kv_t<ValueType>& kv) {
        const Guard<Mutex> guard(__lock);
        if (!kv.key || !__kvdb) [[unlikely]]
            return false;
        fdb_kv kv_{};
        if (find_kv(__kvdb, kv.key, &kv_)) return false;
        fdb_blob blob{};
        return fdb_kv_set_blob(__kvdb, kv.key, fdb_blob_make(&blob, &kv.value, kv.size)) == FDB_NO_ERR;
    }

    Iterator begin();
    Iterator end();
    Iterator cbegin() const;
    Iterator cend() const;

    bool erase(const char* key);
    void clear();
    bool reset();

   private:
    Mutex      __lock;
    fdb_kvdb_t __kvdb;
};

}  // namespace edgelab

#endif
#endif
