#ifndef _MA_DEVICE_H_
#define _MA_DEVICE_H_

#include "core/ma_common.h"
#include "core/transport/ma_transport.h"

namespace ma {

class Device {
public:
    Device();
    ~Device();

    void init();
    void reset();

private:
    const char* m_name;
    uint32_t m_id;
    std::vector<Transport*> m_transports;
};

}  // namespace ma

#endif  // _MA_DEVICE_H_