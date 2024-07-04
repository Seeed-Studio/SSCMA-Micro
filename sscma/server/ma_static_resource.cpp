#include "ma_static_resource.h"

namespace ma {

const static char TAG[] = "ma::StaticResource";

StaticResource::StaticResource() : m_device(nullptr), m_storage(nullptr) {
    m_device     = Device::getInstance();
    m_storage    = m_device->getStorage();
    m_transports = m_device->getTransports();

    MA_LOGD(TAG, "device: %p, storage: %p, transports: %p", m_device, m_storage, m_transports);

    if (m_storage) {

        m_storage->get(MA_STORAGE_KEY_WIFI_SSID, m_wifi.ssid);
        m_storage->get(MA_STORAGE_KEY_WIFI_BSSID, m_wifi.bssid);
        m_storage->get(MA_STORAGE_KEY_WIFI_PWD, m_wifi.password);
        m_storage->get(MA_STORAGE_KEY_WIFI_SECURITY, m_wifi.security);

        MA_LOGD(
            TAG, "wifi: %s %s %s %d", m_wifi.ssid, m_wifi.bssid, m_wifi.password, m_wifi.security);

        m_storage->get(MA_STORAGE_KEY_MQTT_HOST, m_mqtt.host);
        m_storage->get(MA_STORAGE_KEY_MQTT_PORT, m_mqtt.port);
        m_storage->get(MA_STORAGE_KEY_MQTT_CLIENTID, m_mqtt.client_id);
        m_storage->get(MA_STORAGE_KEY_MQTT_USER, m_mqtt.username);
        m_storage->get(MA_STORAGE_KEY_MQTT_PWD, m_mqtt.password);
        m_storage->get(MA_STORAGE_KEY_MQTT_PUB_TOPIC, m_mqtt_topic.pub_topic);
        m_storage->get(MA_STORAGE_KEY_MQTT_SUB_TOPIC, m_mqtt_topic.sub_topic);
        m_storage->get(MA_STORAGE_KEY_MQTT_PUB_QOS, m_mqtt_topic.pub_qos);
        m_storage->get(MA_STORAGE_KEY_MQTT_SUB_QOS, m_mqtt_topic.sub_qos);
        m_storage->get(MA_STORAGE_KEY_MQTT_SSL, m_mqtt.use_ssl);

        MA_LOGD(TAG,
                "mqtt: %s %d %s %s %s %s %d %d",
                m_mqtt.host,
                m_mqtt.port,
                m_mqtt.client_id,
                m_mqtt.username,
                m_mqtt.password,
                m_mqtt_topic.pub_topic,
                m_mqtt_topic.pub_qos,
                m_mqtt_topic.sub_qos);
    }

    // static MQTT mqttTransport(m_mqtt.client_id, m_mqtt_topic.pub_topic, m_mqtt_topic.sub_topic);
    // mqttTransport.connect(
    //     m_mqtt.host, m_mqtt.port, m_mqtt.username, m_mqtt.password, m_mqtt.use_ssl);

    // m_transports->push_front(&mqttTransport);
}


StaticResource::~StaticResource() {}


StaticResource* staticResource = StaticResource::getInstance();


}  // namespace ma