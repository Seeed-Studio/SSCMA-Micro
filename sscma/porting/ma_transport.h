#ifndef _MA_TRANSPORT_H_
#define _MA_TRANSPORT_H_

#include <core/ma_types.h>

#include <cstddef>
#include <cstdint>

namespace ma {

class Transport {
   public:
    Transport(ma_transport_type_t type) : m_initialized(false), m_type(type) {}
    virtual ~Transport() = default;

    Transport(const Transport&)            = delete;
    Transport& operator=(const Transport&) = delete;

    virtual ma_err_t init(void* config) = 0;
    virtual ma_err_t deInit()           = 0;

    operator bool() const { return m_initialized; }

    virtual size_t available() const                                                         = 0;
    virtual size_t send(const char* data, size_t length, int timeout = -1)                   = 0;
    virtual size_t flush()                                                                   = 0;
    virtual size_t receive(char* data, size_t length, int timeout = -1)                      = 0;
    virtual size_t receiveUntil(char* data, size_t length, char delimiter, int timeout = -1) = 0;

   protected:
    bool                m_initialized;
    ma_transport_type_t m_type;
};

}  // namespace ma

#endif  // _MA_TRANSPORT_H_