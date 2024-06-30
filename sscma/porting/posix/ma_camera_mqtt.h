#ifndef MA_CAMERA_MQTT_H_
#define MA_CAMERA_MQTT_H_


#include <vector>

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
    ma_err_t read(ma_img_t* img, int timeout = Tick::waitForever) override;

protected:
    enum MessageType {
        MSG_TYPE_FRAME,
        MSG_TYPE_STATUS,
    };
    void onCallback(mqtt_client_t* client, int type);
    void threadEntry();
    void prase(char* data, size_t length);

private:
    static void threadEntryStub(void* param);
    static void onCallbackStub(mqtt_client_t* client, int type);
    mqtt_client_t* m_client;
    bool m_connected;
    std::string m_txTopic;
    std::string m_rxTopic;
    std::string m_clientID;
    std::string m_username;
    std::string m_password;
    bool m_useSSL;
    Thread* m_thread;
    MessageBox* m_msgBox;
};

}  // namespace ma

#endif  // MA_CAMERA_MQTT_H_