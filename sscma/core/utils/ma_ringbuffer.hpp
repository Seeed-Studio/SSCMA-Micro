#ifndef _MA_RINGBUFFER_H_
#define _MA_RINGBUFFER_H_

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace ma {

template <typename T> class SPSCRingBuffer {
   public:
    explicit SPSCRingBuffer(size_t size) noexcept : m_head(0), m_tail(0), m_size(size), m_buffer(nullptr) {
        static_assert(std::is_trivially_copyable<T>::value);
        static_assert(std::is_trivially_destructible<T>::value);
        assert(size > 0);
        m_buffer = new T[m_size];
        assert(m_buffer);
        std::memset(m_buffer, 0, m_size * sizeof(T));
    }

    ~SPSCRingBuffer() noexcept {
        if (m_buffer) {
            delete[] m_buffer;
            m_buffer = nullptr;
        }
    }

    size_t capacity() const { return m_size; }

    size_t size() const {
        const size_t head = m_head.load();
        const size_t tail = m_tail.load();
        return head >= tail ? head - tail : m_size - tail + head;
    }

    bool empty() const { return size() == 0; }

    size_t push(const T* data, size_t size) noexcept {
        if (!data || size == 0) {
            return 0;
        }
        size_t       head = m_head.load();
        const size_t tail = m_tail.load();
        const size_t free = (head >= tail ? m_size - head + tail : tail - head) - 1;
        if (free == 0) {
            return 0;
        }
        size = size > free ? free : size;
        for (size_t i = 0; i < size; ++i) {
            m_buffer[head] = data[i];
            head           = (head + 1) % m_size;
        }
        m_head.store(head);
        return size;
    }

    size_t pop(T* data, size_t size) noexcept {
        if (!data || size == 0) {
            return 0;
        }
        const size_t head = m_head.load();
        size_t       tail = m_tail.load();
        const size_t used = head >= tail ? head - tail : m_size - tail + head;
        size              = size > used ? used : size;
        for (size_t i = 0; i < size; ++i) {
            data[i] = m_buffer[tail];
            tail    = (tail + 1) % m_size;
        }
        m_tail.store(tail);
        return size;
    }

    size_t popIf(T* data, size_t size, T value) noexcept {
        if (!data || size == 0) {
            return 0;
        }
        const size_t head = m_head.load();
        size_t       tail = m_tail.load();
        const size_t used = head >= tail ? head - tail : m_size - tail + head;
        size_t       i    = tail;
        for (; i < tail + used; ++i) {
            if (m_buffer[i % m_size] == value) {
                break;
            }
        }
        if (i++ == tail + used) {
            return 0;
        }
        size = std::min(size, i - tail);
        for (size_t j = 0; j < size; ++j) {
            data[j] = m_buffer[tail];
            tail    = (tail + 1) % m_size;
        }
        m_tail.store(i % m_size);
        return size;
    }

    void clear() noexcept {
        m_head.store(0);
        m_tail.store(0);
    }

   private:
    alignas(32) std::atomic<size_t> m_head;
    alignas(32) std::atomic<size_t> m_tail;
    const size_t m_size;
    T*           m_buffer;
};

}  // namespace ma

#endif