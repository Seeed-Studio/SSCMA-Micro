#include "ma_ringbuffer.h"

namespace ma::data {

const char* TAG = "ma::utils::RingBuffer";

RingBuffer::RingBuffer(size_t size) noexcept : m_head(0), m_tail(0) {
    m_size   = size;
    m_used   = 0;
    m_buffer = (uint8_t*)ma_malloc(size);
}

RingBuffer::RingBuffer(uint8_t* pool, size_t size) : m_head(0), m_tail(0) {
    m_size   = size;
    m_used   = 0;
    m_buffer = (uint8_t*)pool;
}

RingBuffer::~RingBuffer() noexcept {
    if (m_buffer) {
        ma_free(m_buffer);
    }
}

RingBuffer::operator bool() const {
    return m_buffer != nullptr;
}

size_t RingBuffer::remaining() const {
    return m_used;
}

size_t RingBuffer::read(uint8_t* data, size_t length) {

    MA_ASSERT(data != nullptr && m_buffer != nullptr);

    if (length == 0) {
        return 0;
    }

    if (length > m_used) {
        length = m_used;
    }

    if (length > m_used - m_head) {
        memcpy(data, m_buffer + m_head, m_used - m_head);
        memcpy(data + m_used - m_head, m_buffer, m_head);
    } else {
        memcpy(data, m_buffer + m_head, length);
    }

    m_head = (m_head + length) % m_size;
    m_used -= length;

    return length;
}

size_t RingBuffer::peek(uint8_t* data, size_t length) {

    MA_ASSERT(data != nullptr && m_buffer != nullptr);

    if (length == 0) {
        return 0;
    }

    if (length > m_used) {
        length = m_used;
    }

    if (length > m_used - m_head) {
        memcpy(data, m_buffer + m_head, m_used - m_head);
        memcpy(data + m_used - m_head, m_buffer, m_head);
    } else {
        memcpy(data, m_buffer + m_head, length);
    }

    return length;
}

RingBuffer& operator<<(RingBuffer& rb, uint8_t item) {
    rb.write((uint8_t*)&item, 1);
    return rb;
}

RingBuffer& operator>>(RingBuffer& rb, uint8_t& item) {
    rb.read((uint8_t*)&item, 1);
    return rb;
}

uint8_t RingBuffer::operator[](size_t index) const {
    return m_buffer[(m_head + index) % m_size];
}

size_t RingBuffer::write(const uint8_t* data, size_t length) {

    MA_ASSERT(data != nullptr && m_buffer != nullptr);

    if (length == 0) {
        return 0;
    }

    if (length > m_size - m_used) {
        length = m_size - m_used;
    }

    if (length > m_size - m_tail) {
        memcpy(m_buffer + m_tail, data, m_size - m_tail);
        memcpy(m_buffer, data + m_size - m_tail, length - m_size + m_tail);
    } else {
        memcpy(m_buffer + m_tail, data, length);
    }

    m_tail = (m_tail + length) % m_size;
    m_used += length;
    return length;
}

void RingBuffer::clear() {
    m_head = 0;
    m_tail = 0;
    m_used = 0;
}

}  // namespace ma