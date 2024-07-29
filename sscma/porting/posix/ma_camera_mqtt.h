#ifndef MA_CAMERA_MQTT_H_
#define MA_CAMERA_MQTT_H_


#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <vector>

#include "core/ma_common.h"

#if MA_USE_CAMERA_MQTT

#include "mosquitto.h"

#include "porting/ma_osal.h"

#include "porting/ma_camera.h"

namespace ma {

class CameraMQTT : public Camera {
public:
    CameraMQTT(const char* clientID, const char* txTopic, const char* rxTopic);
    ~CameraMQTT();

    ma_err_t connect(const char* host,
                     int port,
                     const char* username = nullptr,
                     const char* password = nullptr,
                     bool useSSL          = false);
    ma_err_t disconnect();
    ma_err_t open() override;
    ma_err_t close() override;

    operator bool() const override;
    ma_err_t start() override;
    ma_err_t stop() override;
    ma_err_t capture() override;
    ma_err_t read(ma_img_t* img, ma_tick_t timeout = Tick::waitForever) override;
    ma_err_t release(ma_img_t* img) override;

protected:
    void onConnect(struct mosquitto* mosq, int rc);
    void onDisconnect(struct mosquitto* mosq, int rc);
    void onPublish(struct mosquitto* mosq, int mid);
    void onSubscribe(struct mosquitto* mosq, int mid, int qos_count, const int* granted_qos);
    void onUnsubscribe(struct mosquitto* mosq, int mid);
    void onMessage(struct mosquitto* mosq, const struct mosquitto_message* msg);

private:
    static void onConnectStub(struct mosquitto* mosq, void* obj, int rc);
    static void onDisconnectStub(struct mosquitto* mosq, void* obj, int rc);
    static void onPublishStub(struct mosquitto* mosq, void* obj, int mid);
    static void onSubscribeStub(
        struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos);
    static void onUnsubscribeStub(struct mosquitto* mosq, void* obj, int mid);
    static void onMessageStub(struct mosquitto* mosq,
                              void* obj,
                              const struct mosquitto_message* msg);

    struct mosquitto* m_client;
    std::atomic<bool> m_connected;
    std::string m_txTopic;
    std::string m_rxTopic;
    std::string m_clientID;
    std::string m_username;
    std::string m_password;
    bool m_useSSL;
    MessageBox m_msgBox;
    Event m_event;
    Mutex m_mutex;
};


class CameraMQTTProvider : public CameraProvider {
public:
    CameraMQTTProvider(const char* clientID,
                       const char* txTopic,
                       const char* rxTopic,
                       uint32_t shmSize = 4*1024*1024);
    ~CameraMQTTProvider();

    ma_err_t connect(const char* host,
                     int port,
                     const char* username = nullptr,
                     const char* password = nullptr,
                     bool useSSL          = false);
    ma_err_t disconnect();

    operator bool() const override;
    ma_err_t write(const ma_img_t* img) override;

protected:
    void onConnect(struct mosquitto* mosq, int rc);
    void onDisconnect(struct mosquitto* mosq, int rc);
    void onPublish(struct mosquitto* mosq, int mid);
    void onSubscribe(struct mosquitto* mosq, int mid, int qos_count, const int* granted_qos);
    void onUnsubscribe(struct mosquitto* mosq, int mid);
    void onMessage(struct mosquitto* mosq, const struct mosquitto_message* msg);

private:
    static void onConnectStub(struct mosquitto* mosq, void* obj, int rc);
    static void onDisconnectStub(struct mosquitto* mosq, void* obj, int rc);
    static void onPublishStub(struct mosquitto* mosq, void* obj, int mid);
    static void onSubscribeStub(
        struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos);
    static void onUnsubscribeStub(struct mosquitto* mosq, void* obj, int mid);
    static void onMessageStub(struct mosquitto* mosq,
                              void* obj,
                              const struct mosquitto_message* msg);

    struct mosquitto* m_client;
    uint32_t m_count;
    std::atomic<bool> m_connected;
    std::atomic<uint32_t> m_offset;
    std::string m_txTopic;
    std::string m_rxTopic;
    std::string m_clientID;
    std::string m_username;
    std::string m_password;
    MessageBox m_msgBox;
    key_t m_shmKey;
    int m_shmId;
    int m_shmSize;
    void* m_shmAddr;

    bool m_useSSL;
    Mutex m_mutex;
};


}  // namespace ma

#endif

#endif  // MA_CAMERA_MQTT_H_