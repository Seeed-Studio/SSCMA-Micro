#ifndef MA_CAMERA_MQTT_H_
#define MA_CAMERA_MQTT_H_


#include <vector>

#include "core/ma_common.h"

#if MA_USE_CAMERA_MQTT

#include "hv/hv.h"
#include "hv/mqtt_client.h"

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
    void onCallback(mqtt_client_t* client, int type);
    void threadEntry();
    void prase(const char* data, size_t length);

private:
    static void threadEntryStub(void* param);
    static void onCallbackStub(mqtt_client_t* client, int type);
    mqtt_client_t* m_client;
    std::atomic<bool> m_connected;
    std::string m_txTopic;
    std::string m_rxTopic;
    std::string m_clientID;
    std::string m_username;
    std::string m_password;
    bool m_useSSL;
    Thread* m_thread;
    MessageBox* m_msgBox;
    Event m_event;
    Mutex m_mutex;
};


class CameraMQTTProvider : public CameraProvider {
public:
    CameraMQTTProvider(const char* clientID, const char* txTopic, const char* rxTopic);
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
    void onCallback(mqtt_client_t* client, int type);
    void threadEntry();
    void dispatch(const char* data, size_t length);

private:
    static void threadEntryStub(void* param);
    static void onCallbackStub(mqtt_client_t* client, int type);

    mqtt_client_t* m_client;
    uint16_t m_count;
    std::atomic<bool> m_connected;
    std::string m_txTopic;
    std::string m_rxTopic;
    std::string m_clientID;
    std::string m_username;
    std::string m_password;
    bool m_useSSL;
    Thread* m_thread;
    Mutex m_mutex;
};


}  // namespace ma

#endif

#endif  // MA_CAMERA_MQTT_H_