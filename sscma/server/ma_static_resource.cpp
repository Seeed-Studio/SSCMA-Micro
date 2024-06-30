#include "ma_static_resource.h"

namespace ma {

StaticResource::StaticResource() {
    m_device     = Device::getInstance();
    m_storage    = m_device->getStorage();
    m_transports = m_device->getTransports();
}


StaticResource::~StaticResource() {}


StaticResource* staticResource = StaticResource::getInstance();


}  // namespace ma