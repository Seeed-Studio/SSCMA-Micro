#include <algorithm>
#include <cstring>

#include "ma_transport_mqtt.h"

#if MA_USE_TRANSPORT_MQTT

#ifndef MA_MQTT_RECEIVE_BUFFER_SIZE
#define MA_MQTT_RECEIVE_BUFFER_SIZE (4096)
#endif

namespace ma {

const static char* TAG = "ma::transport::mqtt";

void TransportMQTT::onConnect(struct mosquitto* mosq, int rc) {
    m_connected.store(true);
    if (rc == MOSQ_ERR_SUCCESS) {
        mosquitto_subscribe(mosq, nullptr, m_rxTopic.c_str(), 0);
    }
}

void TransportMQTT::onDisconnect(struct mosquitto* mosq, int rc) {
    m_connected.store(false);
}

void TransportMQTT::onMessage(struct mosquitto* mosq, const struct mosquitto_message* msg) {
    m_receiveBuffer.write(static_cast<const char*>(msg->payload), msg->payloadlen);
}

void TransportMQTT::onConnectStub(struct mosquitto* mosq, void* obj, int rc) {
    TransportMQTT* mqtt = static_cast<TransportMQTT*>(obj);
    if (mqtt) {
        mqtt->onConnect(mosq, rc);
    }
}

void TransportMQTT::onDisconnectStub(struct mosquitto* mosq, void* obj, int rc) {
    TransportMQTT* mqtt = static_cast<TransportMQTT*>(obj);
    if (mqtt) {
        mqtt->onDisconnect(mosq, rc);
    }
}
void TransportMQTT::onMessageStub(struct mosquitto* mosq,
                                  void* obj,
                                  const struct mosquitto_message* msg) {

    TransportMQTT* mqtt = static_cast<TransportMQTT*>(obj);
    if (mqtt) {
        mqtt->onMessage(mosq, msg);
    }
}


TransportMQTT::TransportMQTT(const char* clientID, const char* txTopic, const char* rxTopic)
    : Transport(MA_TRANSPORT_MQTT),
      m_client(nullptr),
      m_connected(false),
      m_clientID(clientID),
      m_txTopic(txTopic),
      m_rxTopic(rxTopic) {

    mosquitto_lib_init();

    m_client = mosquitto_new(m_clientID.c_str(), true, this);
    MA_ASSERT(m_client);

    mosquitto_connect_callback_set(m_client, onConnectStub);
    mosquitto_disconnect_callback_set(m_client, onDisconnectStub);
    mosquitto_message_callback_set(m_client, onMessageStub);

    mosquitto_loop_start(m_client);

    m_receiveBuffer.assign(MA_MQTT_RECEIVE_BUFFER_SIZE);
}

TransportMQTT::~TransportMQTT() {

    if (m_client && m_connected.load()) {
        mosquitto_disconnect(m_client);
        mosquitto_loop_stop(m_client, true);
        mosquitto_destroy(m_client);
        m_client = nullptr;
    }

    mosquitto_lib_cleanup();
}

TransportMQTT::operator bool() const {
    return m_connected.load();
}

size_t TransportMQTT::available() const {
    Guard guard(m_mutex);
    return m_receiveBuffer.size();
}

size_t TransportMQTT::send(const char* data, size_t length, int timeout) {
    Guard guard(m_mutex);
    if (!m_client || !m_connected.load()) {
        return 0;
    }
    int ret = mosquitto_publish(m_client, nullptr, m_txTopic.c_str(), length, data, 0, false);
    return ret == MOSQ_ERR_SUCCESS ? length : 0;
}

size_t TransportMQTT::receive(char* data, size_t length, int timeout) {
    Guard guard(m_mutex);
    if (m_receiveBuffer.empty()) {
        return 0;
    }
    size_t ret = m_receiveBuffer.read(data, length);
    return ret;
}

size_t TransportMQTT::receiveUtil(char* data, size_t length, char delimiter, int timeout) {
    Guard guard(m_mutex);
    if (m_receiveBuffer.empty()) {
        return 0;
    }
    return m_receiveBuffer.readUtil(data, length, delimiter);
}

ma_err_t TransportMQTT::connect(
    const char* host, int port, const char* username, const char* password, bool useSSL) {
    Guard guard(m_mutex);
    int ret = 0;
    if (m_connected.load()) {
        return MA_EBUSY;
    }
    if (m_client) {
        if (username && password && username[0] && password[0]) {
            mosquitto_username_pw_set(m_client, username, password);
        }
        mosquitto_reconnect_delay_set(m_client, 2, 30, true);
        ret = mosquitto_connect_async(m_client, host, port, 60);
    }

    return m_client && ret == MOSQ_ERR_SUCCESS ? MA_OK : MA_AGAIN;
}


ma_err_t TransportMQTT::disconnect() {
    Guard guard(m_mutex);
    if (m_client && m_connected.load()) {
        mosquitto_disconnect(m_client);
        m_connected.store(false);
    }
    return MA_OK;
}

}  // namespace ma


#endif
