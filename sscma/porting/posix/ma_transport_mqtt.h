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

class TransportMQTT final : public Transport {
public:
    TransportMQTT(ma_mqtt_config_t* config);

    ~TransportMQTT();


    ma_err_t init(const void* config) noexcept override;
    void deInit() noexcept override;

    size_t available() const noexcept override;
    size_t send(const char* data, size_t length) noexcept override;
    size_t flush() noexcept override;
    size_t receive(char* data, size_t length) noexcept override;
    size_t receiveIf(char* data, size_t length, char delimiter) noexcept override;


protected:
    void onConnect(struct mosquitto* mosq, int rc);
    void onDisconnect(struct mosquitto* mosq, int rc);
    void onMessage(struct mosquitto* mosq, const struct mosquitto_message* msg);


protected:
    static void onConnectStub(struct mosquitto* mosq, void* obj, int rc);
    static void onDisconnectStub(struct mosquitto* mosq, void* obj, int rc);
    static void onMessageStub(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg);

private:
    Mutex m_mutex;
    struct mosquitto* m_client;
    std::atomic<bool> m_connected;
    ma_mqtt_config_t m_config;
    ma_mqtt_topic_config_t m_topicConfig;


    SPSCRingBuffer<char>* m_receiveBuffer;
};

}  // namespace ma


#endif

#endif  // _MA_TRANSPORT_MQTT_H_
