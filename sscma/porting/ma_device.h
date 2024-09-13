#ifndef _MA_DEVICE_H_
#define _MA_DEVICE_H_

#include <core/ma_common.h>

#include <vector>

#include "ma_camera.h"
#include "ma_sensor.h"
#include "ma_storage.h"
#include "ma_transport.h"

namespace ma {

class Device final {
   public:
    static Device* getInstance() noexcept;
    ~Device();

   public:
    const std::string& name() const noexcept { return m_name; }
    const std::string& id() const noexcept { return m_id; }
    const std::string& version() const noexcept { return m_version; }
    size_t             bootCount() const noexcept { return m_bootcount; }

    const std::vector<Transport*>& getTransports() noexcept { return m_transports; }
    const std::vector<Sensor*>&    getSensors() noexcept { return m_sensors; }
    const std::vector<ma_model_t>& getModels() noexcept { return m_models; }
    Storage*                       getStorage() noexcept { return m_storage; }

   private:
    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;

    Device() noexcept;

   private:
    std::string m_name;
    std::string m_id;
    std::string m_version;
    size_t      m_bootcount;

    std::vector<Transport*> m_transports;
    std::vector<Sensor*>    m_sensors;
    std::vector<ma_model_t> m_models;
    Storage*                m_storage;
};

}  // namespace ma

#endif  // _MA_DEVICE_H_