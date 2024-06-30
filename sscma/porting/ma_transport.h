#ifndef _MA_TRANSPORT_H_
#define _MA_TRANSPORT_H_

#include "core/ma_common.h"

namespace ma {

class Transport {
public:
    Transport(ma_transport_type_t type) : m_type(type) {}
    virtual ~Transport() = default;

    virtual operator bool() const = 0;

    virtual size_t available() const                                                       = 0;
    virtual size_t send(const char* data, size_t length, int timeout = -1)                 = 0;
    virtual size_t receive(char* data, size_t length, int timeout = 1)                     = 0;
    virtual size_t receiveUtil(char* data, size_t length, char delimiter, int timeout = 1) = 0;

protected:
    ma_transport_type_t m_type;
};

}  // namespace ma

#endif  // _MA_TRANSPORT_H_