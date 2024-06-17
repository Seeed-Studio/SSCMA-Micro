#ifndef _MA_RINGBUFFER_H_
#define _MA_RINGBUFFER_H_

#include <iostream>
#include <stdexcept>
#include <vector>

#include "porting/ma_osal.h"

namespace ma {

template <typename T>
class ring_buffer {
public:
    explicit ring_buffer(size_t size) noexcept
        : m_buffer(new T[size]), m_head(0), m_tail(0), m_used(0), m_size(size) {}

    explicit ring_buffer() noexcept
        : m_buffer(nullptr), m_head(0), m_tail(0), m_used(0), m_size(0) {}

    ring_buffer(T* pool, size_t size)
        : m_buffer(pool), m_head(0), m_tail(0), m_used(0), m_size(size) {}

    ~ring_buffer() noexcept {
        if (m_buffer) {
            delete[] m_buffer;
        }
    }

    operator bool() const {
        return m_buffer != nullptr;
    }

    void assign(size_t size) {
        if (m_buffer) {
            delete[] m_buffer;
        }
        m_buffer = new T[size];
        m_head   = 0;
        m_tail   = 0;
        m_used   = 0;
        m_size   = size;
    }

    void clear() {
        m_head = 0;
        m_tail = 0;
        m_used = 0;
    }

    size_t capacity() const {
        return m_size;
    }

    size_t size() const {
        return m_used;
    }

    bool empty() const {
        return m_used == 0;
    }

    size_t read(T* data, size_t length) {
        Guard guard(m_mutex);

        if (length > m_used) {
            length = m_used;
        }

        if (length > m_size - m_head) {
            memcpy(data, &m_buffer[m_head], (m_size - m_head) * sizeof(T));
            memcpy(&data[m_size - m_head], &m_buffer[0], length - (m_size - m_head) * sizeof(T));
        } else {
            memcpy(data, &m_buffer[m_head], length * sizeof(T));
        }

        m_head = (m_head + length) % m_size;
        m_used -= length;

        return length;
    }

    size_t peek(T* data, size_t length) const {
        Guard guard(m_mutex);
        if (length > m_used) {
            length = m_used;
        }

        if (length > m_size - m_head) {
            memcpy(data, &m_buffer[m_head], (m_size - m_head) * sizeof(T));
            memcpy(&data[m_size - m_head], &m_buffer[0], length - (m_size - m_head) * sizeof(T));
        } else {
            memcpy(data, &m_buffer[m_head], length * sizeof(T));
        }

        return length;
    }

    size_t write(const T* data, size_t length) {
        Guard guard(m_mutex);
        if (length > m_size - m_used) {
            length = m_size - m_used;
        }

        if (length > m_size - m_tail) {
            memcpy(&m_buffer[m_tail], data, (m_size - m_tail) * sizeof(T));
            memcpy(&m_buffer[0], &data[m_size - m_tail], length - (m_size - m_tail) * sizeof(T));
        } else {
            memcpy(&m_buffer[m_tail], data, length * sizeof(T));
        }

        m_tail = (m_tail + length) % m_size;
        m_used += length;

        return length;
    }

    friend ring_buffer& operator<<(ring_buffer& rb, const T& item) {
        rb.write(&item, 1);
        return rb;
    }

    friend ring_buffer& operator>>(ring_buffer& rb, T& item) {
        rb.read(&item, 1);
        return rb;
    }

    T operator[](size_t index) const {
        size_t actualIndex = (m_head + index) % m_size;
        if (actualIndex >= m_size) {
            actualIndex = 0;
        }
        return m_buffer[actualIndex];
    }

private:
    T* m_buffer;
    size_t m_head;
    size_t m_tail;
    size_t m_used;
    size_t m_size;
    Mutex m_mutex;
};

}  // namespace ma

#endif  // _MA_RINGBUFFER_H_
