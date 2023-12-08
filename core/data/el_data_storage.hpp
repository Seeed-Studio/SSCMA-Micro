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

#include "core/el_config_internal.h"

#if CONFIG_EL_STORAGE

    #include <algorithm>
    #include <cstdarg>
    #include <cstddef>
    #include <cstdio>
    #include <cstring>
    #include <forward_list>
    #include <iterator>
    #include <type_traits>
    #include <utility>

    #include "core/el_debug.h"
    #include "core/el_types.h"
    #include "core/synchronize/el_guard.hpp"
    #include "core/synchronize/el_mutex.hpp"

    #if CONFIG_EL_LIB_FLASHDB
        #include "third_party/FlashDB/flashdb.h"
    #endif

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
          typename std::enable_if<std::is_trivial<ValueTypeNoRef>::value ||
                                  std::is_standard_layout<ValueTypeNoRef>::value>::type* = nullptr>
static inline constexpr el_storage_kv_t<ValueType> el_make_storage_kv(const char* key, ValueType&& value) {
    if constexpr (std::is_same<std::decay<typename std::remove_const<ValueTypeNoRef>::type>, char*>::value)
        return el_storage_kv_t<ValueType>(
          key, std::forward<ValueType>(value), std::strlen(std::forward<ValueType>(value)));

    return el_storage_kv_t<ValueType>(key, std::forward<ValueType>(value));
}

using namespace edgelab::utility;

static constexpr inline uint32_t djb2_hash(const uint8_t* bytes) {
    uint32_t hash = 0x1505;
    uint8_t  byte{};
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
static inline el_storage_kv_t<ValueTypeNoCV> el_make_storage_kv_from_type(VarType&& data) {
    using VarTypeNoCVRef               = typename std::remove_cv<typename std::remove_reference<VarType>::type>::type;
    const char*          type_name     = get_type_name<VarTypeNoCVRef>();
    uint32_t             hash          = djb2_hash(reinterpret_cast<const uint8_t*>(type_name));
    char*                static_buffer = nullptr;
    static const char*   hex_literals  = "0123456789ABCDEF";
    static const char*   key_header    = "edgelab#type_name#";
    static const uint8_t header_size   = std::strlen(key_header);
    static const uint8_t buffer_size   = [&]() -> uint8_t {
        // 8 for hash hex (uint32_t), 1 for terminator, -1 for bit hacks (remove last 1 in binary)
        uint8_t len = static_cast<uint8_t>(header_size + (sizeof(hash) << 1) + 1u - 1u);
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
        static_buffer = new char[buffer_size]{};     // we're not going to delete it
        std::memset(static_buffer, 0, buffer_size);  // not trust initializer
        std::memcpy(static_buffer, key_header, header_size);
        union hex_fmt {
            uint32_t i;
            uint8_t  o[sizeof(uint32_t)];
            hex_fmt(uint32_t n) : i(__builtin_bswap32(n)) {}  // assuming n is little-endian, not platform consistent
        };
        hex_fmt   f{hash};  // wtf snprinf crashes the program, skip use that
        uint16_t* fmt_ptr = reinterpret_cast<uint16_t*>(static_buffer + header_size);
        for (uint8_t i = 0; i < sizeof(decltype(hex_fmt::i)); ++i)
            fmt_ptr[i] = (hex_literals[f.o[i] & 0x0f] << 8) | hex_literals[f.o[i] >> 4];

        hash_list.emplace_front(hash, static_buffer);
    }

    EL_ASSERT(static_buffer != nullptr);
    EL_ASSERT(buffer_size <= FDB_KV_NAME_MAX);

    return el_make_storage_kv(static_buffer, std::forward<VarType>(data));
}

}  // namespace utility

class Storage {
   public:
    [[nodiscard]] static Storage* get_ptr();

    ~Storage();

    Storage(const Storage&)            = delete;
    Storage& operator=(const Storage&) = delete;

    el_err_code_t init(const char* name = CONFIG_EL_STORAGE_NAME, const char* path = CONFIG_EL_STORAGE_PATH);
    void          deinit();

    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
    #if CONFIG_EL_LIB_FLASHDB
        using value_type = const char[FDB_KV_NAME_MAX];
    #else
        using value_type = const char[CONFIG_EL_STORAGE_KEY_SIZE_MAX];
    #endif
        using pointer   = value_type*;
        using reference = value_type&;

    #if CONFIG_EL_LIB_FLASHDB
        explicit Iterator(const Storage* const storage)
            : ___storage(storage), ___kvdb(nullptr), ___iterator(), ___reach_end(true) {
            if (!___storage) return;
            const Guard<Mutex> guard(___storage->__lock);
            ___kvdb = storage->__kvdb;
            if (___kvdb) [[likely]] {
                fdb_kv_iterator_init(___kvdb, &___iterator);
                ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);
            }
        }
    #else
        explicit Iterator(const Storage* const storage) : ___storage(storage), ___reach_end(true) {}
    #endif

        reference operator*() const {
    #if CONFIG_EL_LIB_FLASHDB
            return ___iterator.curr_kv.name;
    #else
            static value_type v{};
            return v;
    #endif
        }

        pointer operator->() const {
    #if CONFIG_EL_LIB_FLASHDB
            return &___iterator.curr_kv.name;
    #else
            static value_type v{};
            return &v;
    #endif
        }

        Iterator& operator++() {
    #if CONFIG_EL_LIB_FLASHDB
            if (!___storage || !___kvdb) [[unlikely]]
                return *this;
            const Guard<Mutex> guard(___storage->__lock);
            ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);
    #endif
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        friend bool operator==(const Iterator& lfs, const Iterator& rhs) {
            return lfs.___reach_end == rhs.___reach_end;
        };

        friend bool operator!=(const Iterator& lfs, const Iterator& rhs) {
            return lfs.___reach_end != rhs.___reach_end;
        }

       protected:
        const Storage* const ___storage;
    #if CONFIG_EL_LIB_FLASHDB
        fdb_kvdb_t      ___kvdb;
        fdb_kv_iterator ___iterator;
    #endif
        bool ___reach_end;
    };

    bool contains(const char* key) const;

    // size of the buffer should be equal to handler->value_len
    template <typename ValueType, typename std::enable_if<!std::is_const<ValueType>::value>::type* = nullptr>
    bool get(types::el_storage_kv_t<ValueType>& kv) const {
        const Guard<Mutex> guard(__lock);
    #if CONFIG_EL_LIB_FLASHDB
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
    #endif

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
        auto                  kv    = utility::el_make_storage_kv(key, value);
        [[maybe_unused]] bool is_ok = get(kv);
        EL_ASSERT(is_ok);

        return value;
    }

    template <typename KVType> Storage& operator>>(KVType&& kv) {
        [[maybe_unused]] bool is_ok = get(std::forward<KVType>(kv));
        EL_ASSERT(is_ok);

        return *this;
    }

    template <typename ValueType> bool emplace(const types::el_storage_kv_t<ValueType>& kv) {
        const Guard<Mutex> guard(__lock);
    #if CONFIG_EL_LIB_FLASHDB
        fdb_blob blob{};
        return fdb_kv_set_blob(__kvdb, kv.key, fdb_blob_make(&blob, &kv.value, kv.size)) == FDB_NO_ERR;
    #else
        return true;
    #endif
    }

    template <typename KVType> Storage& operator<<(KVType&& kv) {
        bool is_ok __attribute__((unused)) = emplace(std::forward<KVType>(kv));
        EL_ASSERT(is_ok);

        return *this;
    }

    template <typename ValueType> bool try_emplace(const types::el_storage_kv_t<ValueType>& kv) {
        const Guard<Mutex> guard(__lock);
    #if CONFIG_EL_LIB_FLASHDB
        if (!kv.key || !__kvdb) [[unlikely]]
            return false;
        fdb_kv kv_{};
        if (find_kv(__kvdb, kv.key, &kv_)) return false;
        fdb_blob blob{};
        return fdb_kv_set_blob(__kvdb, kv.key, fdb_blob_make(&blob, &kv.value, kv.size)) == FDB_NO_ERR;
    #else
        return true;
    #endif
    }

    Iterator begin();
    Iterator end();
    Iterator cbegin() const;
    Iterator cend() const;

    bool erase(const char* key);
    void clear();
    bool reset();

   protected:
    Storage();

   private:
    Mutex __lock;
    #if CONFIG_EL_LIB_FLASHDB
    fdb_kvdb_t __kvdb;
    #endif
};

}  // namespace edgelab

#endif

#endif
