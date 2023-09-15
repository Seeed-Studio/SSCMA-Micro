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

#include "el_data_storage.hpp"

#ifdef CONFIG_EL_LIB_FLASHDB

namespace edgelab {

Storage::Storage() : __lock(), __kvdb(new fdb_kvdb{}) { EL_ASSERT(__kvdb); }

Storage::~Storage() { deinit(); };

el_err_code_t Storage::init(const char* name, const char* path) {
    const Guard<Mutex> guard(__lock);
    return fdb_kvdb_init(__kvdb, name, path, nullptr, nullptr) == FDB_NO_ERR ? EL_OK : EL_EINVAL;
}

void Storage::deinit() {
    const Guard<Mutex> guard(__lock);
    if (__kvdb && (fdb_kvdb_deinit(__kvdb) == FDB_NO_ERR)) [[likely]] {
        delete __kvdb;
        __kvdb = nullptr;
    }
}

bool Storage::contains(const char* key) const {
    const Guard<Mutex> guard(__lock);
    if (!key || !__kvdb) [[unlikely]]
        return false;
    fdb_kv kv{};
    return find_kv(__kvdb, key, &kv);
}

size_t Storage::get_value_size(const char* key) const {
    const Guard<Mutex> guard(__lock);
    fdb_kv             handler{};
    fdb_kv_t           p_handler = fdb_kv_get_obj(__kvdb, key, &handler);
    if (!p_handler || !p_handler->value_len) [[unlikely]]
        return 0u;

    return p_handler->value_len;
}

struct Storage::Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = const char[FDB_KV_NAME_MAX];
    using pointer           = value_type*;
    using reference         = value_type&;

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

    reference operator*() const { return ___iterator.curr_kv.name; }

    pointer operator->() const { return &___iterator.curr_kv.name; }

    Iterator& operator++() {
        if (!___storage || !___kvdb) [[unlikely]]
            return *this;
        const Guard<Mutex> guard(___storage->__lock);
        ___reach_end = !fdb_kv_iterate(___kvdb, &___iterator);
        return *this;
    }

    Iterator operator++(int) {
        Iterator tmp = *this;
        ++(*this);
        return tmp;
    }

    friend bool operator==(const Iterator& lfs, const Iterator& rhs) { return lfs.___reach_end == rhs.___reach_end; };

    friend bool operator!=(const Iterator& lfs, const Iterator& rhs) { return lfs.___reach_end != rhs.___reach_end; }

   protected:
    const Storage* const ___storage;
    fdb_kvdb_t           ___kvdb;
    fdb_kv_iterator      ___iterator;
    bool                 ___reach_end;
};

Storage::Iterator Storage::begin() { return Iterator(this); }
Storage::Iterator Storage::end() { return Iterator(nullptr); }
Storage::Iterator Storage::cbegin() const { return Iterator(this); }
Storage::Iterator Storage::cend() const { return Iterator(nullptr); }

bool Storage::erase(const char* key) {
    const Guard<Mutex> guard(__lock);
    return fdb_kv_del(__kvdb, key) == FDB_NO_ERR;
}

void Storage::clear() {
    const Guard<Mutex> guard(__lock);
    if (!__kvdb) [[unlikely]]
        return;
    struct fdb_kv_iterator iterator;
    fdb_kv_iterator_init(__kvdb, &iterator);
    while (fdb_kv_iterate(__kvdb, &iterator)) fdb_kv_del(__kvdb, iterator.curr_kv.name);
}

bool Storage::reset() {
    const Guard<Mutex> guard(__lock);
    return __kvdb ? fdb_kv_set_default(__kvdb) == FDB_NO_ERR : false;
}

}  // namespace edgelab

#endif
