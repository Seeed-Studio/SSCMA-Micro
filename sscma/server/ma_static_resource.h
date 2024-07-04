#ifndef _MA_STATIC_RESOURCE_H_
#define _MA_STATIC_RESOURCE_H_

#include <forward_list>

#include <core/ma_core.h>
#include <porting/ma_porting.h>

namespace ma {

class StaticResource final {
public:
    StaticResource();
    ~StaticResource();

    static inline StaticResource* getInstance() {
        static StaticResource instance;
        return &instance;
    }

    Device* m_device;
    Storage* m_storage;
    std::forward_list<Transport*> m_transports;
    ma_wifi_config_t m_wifi;
    ma_mqtt_config_t m_mqtt;
    ma_mqtt_topic_config_t m_mqtt_topic;
};

extern StaticResource* staticResource;

}  // namespace ma

#endif  // _MA_STATIC_RESOURCE_H_