#ifndef _MA_DEVICE_H_
#define _MA_DEVICE_H_

#include <core/ma_common.h>

#include <forward_list>

#include "ma_camera.h"
#include "ma_storage.h"
#include "ma_transport.h"

namespace ma {

class Device final {
   private:
    Device();

   public:
    static Device* getInstance();
    ~Device();

   public:
    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;

    std::string name() const { return m_name; }

    std::string id() const { return m_id; }

    std::string version() const { return m_version; }

    const std::forward_list<Transport*>& getTransports() { return m_transports; }

    Storage* getStorage() { return m_storage; }

    const std::forward_list<Camera*>& getCameras() { return m_cameras; }

    const std::forward_list<ma_model_t>& getModels() { return m_models; }

   private:
    std::string m_name;
    std::string m_id;
    std::string m_version;

    std::forward_list<Transport*> m_transports;
    std::forward_list<Camera*>    m_cameras;
    std::forward_list<ma_model_t> m_models;
    Storage*                      m_storage;
};

}  // namespace ma

#endif  // _MA_DEVICE_H_