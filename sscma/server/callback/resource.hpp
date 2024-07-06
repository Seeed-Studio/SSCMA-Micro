#pragma once

#include "core/ma_core.h"
#include "porting/ma_porting.h"

using namespace ma;


namespace ma::server::callback {

constexpr char TAG[] = "ma::server::callback::resource";

class staticResource final {
public:
    staticResource() {

        device  = Device::getInstance();
        storage = device->getStorage();

        transports.assign(device->getTransports().begin(), device->getTransports().end());

        executor = new Executor(MA_SEVER_AT_EXECUTOR_STACK_SIZE, MA_SEVER_AT_EXECUTOR_TASK_PRIO);
        MA_ASSERT(executor);

        static EngineDefault v_engine;
        engine = &v_engine;
    }
    ~staticResource() = default;

    static staticResource* getInstance() {
        static staticResource resource;
        return &resource;
    }

    ma_err_t init() {

        storage->get(MA_STORAGE_KEY_MQTT_HOST, m_mqttConfig.host, "");

        if (m_mqttConfig.host[0] != '\0') {

            storage->get(MA_STORAGE_KEY_MQTT_PORT, m_mqttConfig.port, 1883);
            storage->get(MA_STORAGE_KEY_MQTT_CLIENTID, m_mqttConfig.client_id, "");
            storage->get(MA_STORAGE_KEY_MQTT_USER, m_mqttConfig.username, "");
            storage->get(MA_STORAGE_KEY_MQTT_PWD, m_mqttConfig.password, "");

            if (m_mqttConfig.client_id[0] == '\0') {
                snprintf(m_mqttConfig.client_id,
                         MA_MQTT_MAX_CLIENT_ID_LENGTH,
                         MA_MQTT_CLIENTID_FMT,
                         device->name().c_str(),
                         device->id().c_str());
                storage->set(MA_STORAGE_KEY_MQTT_CLIENTID, m_mqttConfig.client_id);
            }

            snprintf(m_mqttTopicConfig.pub_topic,
                     MA_MQTT_MAX_TOPIC_LENGTH,
                     MA_MQTT_TOPIC_FMT "/tx",
                     MA_AT_API_VERSION,
                     m_mqttConfig.client_id);
            snprintf(m_mqttTopicConfig.sub_topic,
                     MA_MQTT_MAX_TOPIC_LENGTH,
                     MA_MQTT_TOPIC_FMT "/rx",
                     MA_AT_API_VERSION,
                     m_mqttConfig.client_id);

#if MA_USE_TRANSPORT_MQTT
            MQTT* transport = new MQTT(
                m_mqttConfig.client_id, m_mqttTopicConfig.pub_topic, m_mqttTopicConfig.sub_topic);
            MA_LOGD(TAG, "MQTT Transport: %p", transport);
            transport->connect(
                m_mqttConfig.host, m_mqttConfig.port, m_mqttConfig.username, m_mqttConfig.password);

            transports.push_front(transport);
#endif
        }


        MA_LOGD(TAG, "MQTT Host: %s:%d", m_mqttConfig.host, m_mqttConfig.port);
        MA_LOGD(TAG, "MQTT Client ID: %s", m_mqttConfig.client_id);
        MA_LOGD(TAG, "MQTT User: %s", m_mqttConfig.username);
        MA_LOGD(TAG, "MQTT Pwd: %s", m_mqttConfig.password);
        MA_LOGD(TAG, "MQTT Pub Topic: %s", m_mqttTopicConfig.pub_topic);
        MA_LOGD(TAG, "MQTT Sub Topic: %s", m_mqttTopicConfig.sub_topic);

        storage->get(MA_STORAGE_KEY_WIFI_SSID, m_wifiConfig.ssid, "");
        storage->get(MA_STORAGE_KEY_WIFI_BSSID, m_wifiConfig.bssid, "");
        storage->get(MA_STORAGE_KEY_WIFI_PWD, m_wifiConfig.password, "");
        storage->get(MA_STORAGE_KEY_WIFI_SECURITY, m_wifiConfig.security, SEC_AUTO);

        MA_LOGD(TAG, "Wifi SSID: %s", m_wifiConfig.ssid);
        MA_LOGD(TAG, "Wifi BSSID: %s", m_wifiConfig.bssid);
        MA_LOGD(TAG, "Wifi Pwd: %s", m_wifiConfig.password);
        MA_LOGD(TAG, "Wifi Security: %d", m_wifiConfig.security);

        return MA_OK;
    }

public:
    Executor* executor;
    Device* device;
    Storage* storage;
    Engine* engine;
    std::forward_list<Transport*> transports;


    // internal configs that stored in flash
    int32_t boot_count;

    // internal states
    std::atomic<bool> is_ready;
    std::atomic<bool> is_sample;
    std::atomic<bool> is_invoke;
    
    // internal states
    std::atomic<std::size_t> current_task_id;

    // configs
    ma_wifi_config_t m_wifiConfig;
    ma_mqtt_config_t m_mqttConfig;
    ma_mqtt_topic_config_t m_mqttTopicConfig;
};

static staticResource* static_resource{staticResource::getInstance()};

}  // namespace ma::server::callback