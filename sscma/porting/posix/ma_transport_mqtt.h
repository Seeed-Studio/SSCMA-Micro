#ifndef _MA_TRANSPORT_MQTT_H_
#define _MA_TRANSPORT_MQTT_H_

#include "core/ma_common.h"

#if MA_USE_TRANSPORT_MQTT

#include <atomic>
#include <vector>

#include "mosquitto.h"

#include "core/utils/ma_ringbuffer.hpp"

#include "porting/ma_osal.h"
#include "porting/ma_transport.h"

namespace ma {

class MQTT final : public Transport {
public:
    MQTT(const char* clientID, const char* txTopic, const char* rxTopic);

    ~MQTT() override;

    operator bool() const override;

    size_t available() const override;
    size_t send(const char* data, size_t length, int timeout = -1) override;
    size_t receive(char* data, size_t length, int timeout = 1) override;
    size_t receiveUtil(char* data, size_t length, char delimiter, int timeout = 1) override;

    ma_err_t connect(const std::string& host,
                     int port,
                     const std::string& username = "",
                     const std::string& password = "",
                     bool useSSL                 = false) {
        return connect(host.c_str(), port, username.c_str(), password.c_str(), useSSL);
    }

    ma_err_t connect(const char* host,
                     int port,
                     const char* username = nullptr,
                     const char* password = nullptr,
                     bool useSSL          = false);

    ma_err_t disconnect();


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
    Mutex m_mutex;
    std::string m_txTopic;
    std::string m_rxTopic;
    std::string m_clientID;
    std::string m_username;
    std::string m_password;
    bool m_useSSL;

    ring_buffer<char> m_receiveBuffer;
};

}  // namespace ma


#endif

#endif  // _MA_TRANSPORT_MQTT_H_
