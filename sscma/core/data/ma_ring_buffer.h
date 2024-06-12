#ifndef _MA_RING_BUFFER_H_
#define _MA_RING_BUFFER_H_

#include <iostream>
#include <vector>

#include "core/ma_common.h"

namespace ma::data {

class RingBuffer {
public:
    explicit RingBuffer(size_t size) noexcept;
    RingBuffer(uint8_t* pool, size_t size);
    ~RingBuffer() noexcept;
    operator bool() const;
    void               clear();
    size_t             capacity() const;
    size_t             remaining() const;
    size_t             read(uint8_t* data, size_t length);
    size_t             peek(uint8_t* data, size_t length);
    size_t             write(const uint8_t* data, size_t length);

    friend RingBuffer& operator<<(RingBuffer& rb, uint8_t item);
    friend RingBuffer& operator>>(RingBuffer& rb, uint8_t& item);
    uint8_t            operator[](size_t index) const;

private:
    uint8_t* m_buffer;
    size_t   m_head;
    size_t   m_tail;
    size_t   m_used;
    size_t   m_size;
};

}  // namespace ma::data

#endif  // _MA_RING_BUFFER_H_