

#include <cstring>

#include <core/ma_common.h>
#include <porting/ma_porting.h>

#include "ma_camera_sg200x.h"

namespace ma {

Device::Device() noexcept {

    MA_LOGD(MA_TAG, "Initializing device: %s", MA_BOARD_NAME);
    { m_name = MA_BOARD_NAME; }

    MA_LOGD(MA_TAG, "Initializing device version");
    { m_version = "SG200X"; }

    MA_LOGD(MA_TAG, "Initializing device ID");
    { m_id = "SG200X"; }

    MA_LOGD(MA_TAG, "Initializing storage");
    {
        static StorageFile storage;
        if (storage.init(MA_CONFIG_FILE) != MA_OK) {
            ma_abort();
        }
        m_storage = &storage;
    }

    MA_LOGD(MA_TAG, "Initializing transports");
    {
        ma_mqtt_config_t mqttConfig;
        ma_mqtt_topic_config_t mqttTopicConfig;
        std::memset(&mqttConfig, 0, sizeof(mqttConfig));
        std::memset(&mqttTopicConfig, 0, sizeof(mqttTopicConfig));


        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_HOST, mqttConfig.host, "localhost");
        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_CLIENTID, mqttConfig.client_id, MA_BOARD_NAME);
        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_USER, mqttConfig.username, "");
        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_PWD, mqttConfig.password, "");
        MA_STORAGE_GET_POD(m_storage, MA_STORAGE_KEY_MQTT_PORT, mqttConfig.port, 1883);
        MA_STORAGE_GET_POD(m_storage, MA_STORAGE_KEY_MQTT_SSL, mqttConfig.use_ssl, 0);

        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_PUB_TOPIC, mqttTopicConfig.pub_topic, "tx");
        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_SUB_TOPIC, mqttTopicConfig.sub_topic, "rx");
        MA_STORAGE_GET_POD(m_storage, MA_STORAGE_KEY_MQTT_PUB_QOS, mqttTopicConfig.pub_qos, 0);
        MA_STORAGE_GET_POD(m_storage, MA_STORAGE_KEY_MQTT_SUB_QOS, mqttTopicConfig.sub_qos, 0);

        static TransportMQTT transportMqtt(&mqttConfig);

        if (transportMqtt.init(&mqttTopicConfig) != MA_OK) {
            ma_abort();
        }

        m_transports.push_back(&transportMqtt);
    }

    MA_LOGD(MA_TAG, "Initializing camera");
    {
        static CameraSG200X camera(0);
        m_sensors.push_back(&camera);
    }

    MA_LOGD(MA_TAG, "Initializing device done");
}

Device* Device::getInstance() noexcept {
    static Device device{};
    return &device;
}

Device::~Device() {}

}  // namespace ma