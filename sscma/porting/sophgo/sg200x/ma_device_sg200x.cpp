

#include <cstring>
#include <filesystem>

#include <core/ma_common.h>
#include <porting/ma_porting.h>

#include "ma_camera_sg200x.h"

namespace fs = std::filesystem;

namespace ma {

Device::Device() noexcept {

    MA_LOGD(MA_TAG, "Initializing device: %s", MA_BOARD_NAME);
    { m_name = MA_BOARD_NAME; }

    MA_LOGD(MA_TAG, "Initializing device version");
    { m_version = "v1"; }

    MA_LOGD(MA_TAG, "Initializing device ID");
    {
        std::ifstream file("/sys/class/cvi-base/base_efuse_shadow", std::ios::binary);
        if (!file.is_open()) {
            MA_LOGW(MA_TAG, "Failed to open /sys/class/cvi-base/base_efuse_shadow");
            m_id = "unknown";
        } else {
            file.seekg(0x48, std::ios::beg);

            // 8 bytes hex string
            char id[8];
            file.read(id, 8);
            if (file.gcount() != 8) {
                MA_LOGW(MA_TAG, "Failed to read /sys/class/cvi-base/base_efuse_shadow");
                m_id = "unknown";
            } else {
                for (int i = 0; i < 8; i++) {
                    char buf[3];
                    snprintf(buf, sizeof(buf), "%02x", id[i]);
                    m_id += buf;
                }
            }

            file.close();
        }
    }

    MA_LOGD(MA_TAG, "Initializing storage");
    {
        static StorageFile storage;
        if (storage.init(MA_CONFIG_FILE) != MA_OK) {
            ma_abort();
        }
        m_storage = &storage;
    }

    MA_LOGD(MA_TAG, "Initializing models");
    {
        if (fs::exists(MA_MODEL_DIR)) {
            size_t id = 0;
            for (const auto& entry : fs::directory_iterator(MA_MODEL_DIR)) {
                if (fs::is_regular_file(entry) && entry.path().extension() == ".cvimodel") {
                    ma_model_t model;
                    model.id   = ++id;
                    model.type = MA_MODEL_TYPE_UNDEFINED;
                    char* name = new char[entry.path().filename().string().size() + 1];
                    std::strcpy(name, entry.path().filename().string().c_str());
                    model.name = name;
                    char* addr = new char[entry.path().string().size() + 1];
                    std::strcpy(addr, entry.path().string().c_str());
                    model.addr = addr;
                    model.size = fs::file_size(entry.path());
                    m_models.push_back(model);
                }
            }
        }
    }

    MA_LOGD(MA_TAG, "Initializing transports");
    {
#ifdef MA_USE_TRANSPORT_MQTT
        ma_mqtt_config_t mqttConfig;
        ma_mqtt_topic_config_t mqttTopicConfig;
        std::memset(&mqttConfig, 0, sizeof(mqttConfig));
        std::memset(&mqttTopicConfig, 0, sizeof(mqttTopicConfig));

        static char buf[256];


        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_HOST, mqttConfig.host, "localhost");

        snprintf(buf, sizeof(buf), MA_MQTT_CLIENTID_FMT, MA_BOARD_NAME, m_id.c_str());
        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_CLIENTID, mqttConfig.client_id, buf);

        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_USER, mqttConfig.username, "");
        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_PWD, mqttConfig.password, "");
        MA_STORAGE_GET_POD(m_storage, MA_STORAGE_KEY_MQTT_PORT, mqttConfig.port, 1883);
        MA_STORAGE_GET_POD(m_storage, MA_STORAGE_KEY_MQTT_SSL, mqttConfig.use_ssl, 0);

        snprintf(buf, sizeof(buf), MA_MQTT_TOPIC_FMT, mqttConfig.client_id, "tx");
        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_PUB_TOPIC, mqttTopicConfig.pub_topic, buf);
        snprintf(buf, sizeof(buf), MA_MQTT_TOPIC_FMT, mqttConfig.client_id, "rx");
        MA_STORAGE_GET_ASTR(m_storage, MA_STORAGE_KEY_MQTT_SUB_TOPIC, mqttTopicConfig.sub_topic, buf);
        MA_STORAGE_GET_POD(m_storage, MA_STORAGE_KEY_MQTT_PUB_QOS, mqttTopicConfig.pub_qos, 0);
        MA_STORAGE_GET_POD(m_storage, MA_STORAGE_KEY_MQTT_SUB_QOS, mqttTopicConfig.sub_qos, 0);

        static TransportMQTT transportMqtt(&mqttConfig);

        if (transportMqtt.init(&mqttTopicConfig) != MA_OK) {
            ma_abort();
        }

        m_transports.push_back(&transportMqtt);
#endif
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