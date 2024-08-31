#pragma once

#include "core/ma_core.h"
#include "porting/ma_porting.h"

using namespace ma;

namespace ma::server::callback {

using namespace ma::engine;

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
#if MA_USE_FILESYSTEM
        MA_STORAGE_GET_ASTR(storage, MA_STORAGE_KEY_MODEL_DIR, model_path, MA_SSCMA_MODEL_DEFAULT_PATH);
        Engine::findModels(model_path, 0);
#endif
        MA_STORAGE_GET_STR(storage, MA_STORAGE_KEY_ID, device->id(), device->id());
        MA_STORAGE_GET_POD(storage, MA_STORAGE_KEY_MODEL_ID, cur_model_id, 0);
        MA_STORAGE_GET_ASTR(storage, MA_STORAGE_KEY_MQTT_HOST, mqtt_cfg.host, "");

        if (mqtt_cfg.host[0] != '\0') {
            MA_STORAGE_GET_POD(storage, MA_STORAGE_KEY_MQTT_PORT, mqtt_cfg.port, 1883);
            MA_STORAGE_GET_ASTR(storage, MA_STORAGE_KEY_MQTT_CLIENTID, mqtt_cfg.client_id, "");
            MA_STORAGE_GET_ASTR(storage, MA_STORAGE_KEY_MQTT_USER, mqtt_cfg.username, "");
            MA_STORAGE_GET_ASTR(storage, MA_STORAGE_KEY_MQTT_PWD, mqtt_cfg.password, "");

            if (mqtt_cfg.client_id[0] == '\0') {
                snprintf(mqtt_cfg.client_id,
                         MA_MQTT_MAX_CLIENT_ID_LENGTH - 1,
                         MA_MQTT_CLIENTID_FMT,
                         device->name().c_str(),
                         device->id().c_str());
                MA_STORAGE_NOSTA_SET_ASTR(storage, MA_STORAGE_KEY_MQTT_CLIENTID, mqtt_cfg.client_id);
            }

            snprintf(mqtt_topic.pub_topic,
                     MA_MQTT_MAX_TOPIC_LENGTH - 1,
                     MA_MQTT_TOPIC_FMT "/tx",
                     MA_AT_API_VERSION,
                     mqtt_cfg.client_id);
            snprintf(mqtt_topic.sub_topic,
                     MA_MQTT_MAX_TOPIC_LENGTH - 1,
                     MA_MQTT_TOPIC_FMT "/rx",
                     MA_AT_API_VERSION,
                     mqtt_cfg.client_id);

#if MA_USE_TRANSPORT_MQTT
            TransportMQTT* transport =
              new TransportMQTT(mqtt_cfg.client_id, mqtt_topic.pub_topic, mqtt_topic.sub_topic);
            MA_LOGD(TAG, "TransportMQTT Transport: %p", transport);
            transport->connect(mqtt_cfg.host, mqtt_cfg.port, mqtt_cfg.username, mqtt_cfg.password);

            transports.push_front(transport);
#endif
        }

        MA_LOGD(TAG, "TransportMQTT Host: %s:%d", mqtt_cfg.host, mqtt_cfg.port);
        MA_LOGD(TAG, "TransportMQTT Client ID: %s", mqtt_cfg.client_id);
        MA_LOGD(TAG, "TransportMQTT User: %s", mqtt_cfg.username);
        MA_LOGD(TAG, "TransportMQTT Pwd: %s", mqtt_cfg.password);
        MA_LOGD(TAG, "TransportMQTT Pub Topic: %s", mqtt_topic.pub_topic);
        MA_LOGD(TAG, "TransportMQTT Sub Topic: %s", mqtt_topic.sub_topic);

        MA_STORAGE_GET_ASTR(storage, MA_STORAGE_KEY_WIFI_SSID, wifi_cfg.ssid, "");
        MA_STORAGE_GET_ASTR(storage, MA_STORAGE_KEY_WIFI_BSSID, wifi_cfg.bssid, "");
        MA_STORAGE_GET_ASTR(storage, MA_STORAGE_KEY_WIFI_PWD, wifi_cfg.password, "");
        MA_STORAGE_GET_POD(storage, MA_STORAGE_KEY_WIFI_SECURITY, wifi_cfg.security, SEC_AUTO);

        MA_LOGD(TAG, "Wifi SSID: %s", wifi_cfg.ssid);
        MA_LOGD(TAG, "Wifi BSSID: %s", wifi_cfg.bssid);
        MA_LOGD(TAG, "Wifi Pwd: %s", wifi_cfg.password);
        MA_LOGD(TAG, "Wifi Security: %d", wifi_cfg.security);

        return MA_OK;
    }

   public:
    Executor*                     executor;
    Device*                       device;
    Storage*                      storage;
    Engine*                       engine;
    std::forward_list<Transport*> transports;

#if MA_USE_FILESYSTEM
    char model_path[MA_MODEL_MAX_PATH_LENGTH];
#endif

    // internal configs that stored in flash
    int32_t boot_count;
    uint8_t cur_model_id;

    // internal states
    std::atomic<bool> is_ready;
    std::atomic<bool> is_sample;
    std::atomic<bool> is_invoke;

    // internal states
    std::atomic<std::size_t> current_task_id;

    // configs
    ma_wifi_config_t       wifi_cfg;
    ma_mqtt_config_t       mqtt_cfg;
    ma_mqtt_topic_config_t mqtt_topic;
};

static staticResource* static_resource{staticResource::getInstance()};

}  // namespace ma::server::callback