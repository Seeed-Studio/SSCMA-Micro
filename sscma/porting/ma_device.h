#ifndef _MA_DEVICE_H_
#define _MA_DEVICE_H_

#include <forward_list>

#include "core/ma_common.h"

#include "ma_camera.h"
#include "ma_storage.h"
#include "ma_transport.h"

namespace ma {

class Device {
public:
    // singleton
    Device()                         = default;
    virtual ~Device()                = default;
    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;

    virtual ma_err_t init()   = 0;
    virtual ma_err_t deinit() = 0;

    static Device* getInstance();

    std::string name() const {
        return m_name;
    }
    std::string id() const {
        return m_id;
    }
    std::string version() const {
        return m_version;
    }

    std::forward_list<Transport*>& getTransports() {
        return m_transports;
    }
    Storage* getStorage() {
        return m_storage;
    }

protected:
    std::forward_list<Transport*> m_transports;
    std::forward_list<Camera*> m_cameras;
    Storage* m_storage;
    std::string m_name;
    std::string m_id;
    std::string m_version;

private:
    static Device* s_instance;
};


}  // namespace ma

#endif  // _MA_DEVICE_H_