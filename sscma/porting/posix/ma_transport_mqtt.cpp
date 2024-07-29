#include <algorithm>
#include <cstring>

#include "ma_transport_mqtt.h"

#if MA_USE_TRANSPORT_MQTT

namespace ma {

const static char* TAG = "ma::transport::mqtt";


void MQTT::onConnect(struct mosquitto* mosq, int rc) {
    MA_LOGD(TAG, "onConnect: %d", rc);
    m_connected.store(true);

    if (rc == MOSQ_ERR_SUCCESS) {
        mosquitto_subscribe(mosq, nullptr, m_rxTopic.c_str(), 0);
    }
}

void MQTT::onDisconnect(struct mosquitto* mosq, int rc) {
    MA_LOGD(TAG, "onDisconnect: %d", rc);
    m_connected.store(false);
}

void MQTT::onPublish(struct mosquitto* mosq, int mid) {
    MA_LOGD(TAG, "onPublish: %d", mid);
}

void MQTT::onSubscribe(struct mosquitto* mosq, int mid, int qos_count, const int* granted_qos) {
    MA_LOGD(TAG, "onSubscribe: %d", mid);
}

void MQTT::onUnsubscribe(struct mosquitto* mosq, int mid) {
    MA_LOGD(TAG, "onUnsubscribe: %d", mid);
}

void MQTT::onMessage(struct mosquitto* mosq, const struct mosquitto_message* msg) {
    MA_LOGD(TAG, "onMessage: %d", msg->mid);
    m_receiveBuffer.write(static_cast<const char*>(msg->payload), msg->payloadlen);
}

void MQTT::onConnectStub(struct mosquitto* mosq, void* obj, int rc) {
    MQTT* mqtt = static_cast<MQTT*>(obj);
    if (mqtt) {
        mqtt->onConnect(mosq, rc);
    }
}

void MQTT::onDisconnectStub(struct mosquitto* mosq, void* obj, int rc) {

    MQTT* mqtt = static_cast<MQTT*>(obj);
    if (mqtt) {
        mqtt->onDisconnect(mosq, rc);
    }
}

void MQTT::onPublishStub(struct mosquitto* mosq, void* obj, int mid) {

    MQTT* mqtt = static_cast<MQTT*>(obj);
    if (mqtt) {
        mqtt->onPublish(mosq, mid);
    }
}

void MQTT::onSubscribeStub(
    struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos) {

    MQTT* mqtt = static_cast<MQTT*>(obj);
    if (mqtt) {
        mqtt->onSubscribe(mosq, mid, qos_count, granted_qos);
    }
}

void MQTT::onUnsubscribeStub(struct mosquitto* mosq, void* obj, int mid) {

    MQTT* mqtt = static_cast<MQTT*>(obj);
    if (mqtt) {
        mqtt->onUnsubscribe(mosq, mid);
    }
}

void MQTT::onMessageStub(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg) {

    MQTT* mqtt = static_cast<MQTT*>(obj);
    if (mqtt) {
        mqtt->onMessage(mosq, msg);
    }
}


MQTT::MQTT(const char* clientID, const char* txTopic, const char* rxTopic)
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
    mosquitto_publish_callback_set(m_client, onPublishStub);
    mosquitto_subscribe_callback_set(m_client, onSubscribeStub);
    mosquitto_unsubscribe_callback_set(m_client, onUnsubscribeStub);
    mosquitto_message_callback_set(m_client, onMessageStub);

    mosquitto_loop_start(m_client);

    m_receiveBuffer.assign(MA_MQTT_RECEIVE_BUFFER_SIZE);
}

MQTT::~MQTT() {

    if (m_client && m_connected.load()) {
        mosquitto_disconnect(m_client);
        mosquitto_loop_stop(m_client, true);
        mosquitto_destroy(m_client);
        m_client = nullptr;
    }

    mosquitto_lib_cleanup();
}

MQTT::operator bool() const {
    return m_connected.load();
}

size_t MQTT::available() const {
    Guard guard(m_mutex);
    return m_receiveBuffer.size();
}

size_t MQTT::send(const char* data, size_t length, int timeout) {
    Guard guard(m_mutex);
    if (!m_client || !m_connected.load()) {
        return 0;
    }
    int ret = mosquitto_publish(m_client, nullptr, m_txTopic.c_str(), length, data, 0, false);
    return ret == MOSQ_ERR_SUCCESS ? length : 0;
}

size_t MQTT::receive(char* data, size_t length, int timeout) {
    Guard guard(m_mutex);
    if (m_receiveBuffer.empty()) {
        return 0;
    }
    size_t ret = m_receiveBuffer.read(data, length);
    return ret;
}

size_t MQTT::receiveUtil(char* data, size_t length, char delimiter, int timeout) {
    Guard guard(m_mutex);
    if (m_receiveBuffer.empty()) {
        return 0;
    }
    return m_receiveBuffer.readUtil(data, length, delimiter);
}

ma_err_t MQTT::connect(
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


ma_err_t MQTT::disconnect() {
    Guard guard(m_mutex);
    if (m_client && m_connected.load()) {
        mosquitto_disconnect(m_client);
        m_connected.store(false);
    }
    return MA_OK;
}

}  // namespace ma


#endif
