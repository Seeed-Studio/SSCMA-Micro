#include <algorithm>
#include <cstring>

#include "ma_transport_mqtt.h"

#if MA_USE_TRANSPORT_MQTT

namespace ma {

constexpr char TAG[] = "ma::transport::mqtt";

void MQTT::onCallbackStub(mqtt_client_t* cli, int type) {
    MQTT* mqtt = reinterpret_cast<MQTT*>(mqtt_client_get_userdata(cli));
    if (mqtt) {
        mqtt->onCallback(cli, type);
    }
}

void MQTT::onCallback(mqtt_client_t* cli, int type) {
    switch (type) {
        case MQTT_TYPE_CONNECT:
            m_connected.store(true);
            break;
        case MQTT_TYPE_CONNACK:
            mqtt_client_subscribe(m_client, m_rxTopic.c_str(), 0);
            break;
        case MQTT_TYPE_DISCONNECT:
            m_connected.store(false);
            break;
        case MQTT_TYPE_PUBLISH:
            m_receiveBuffer.write(cli->message.payload, cli->message.payload_len);
            break;
        default:
            break;
    }
}

void MQTT::threadEntryStub(void* param) {
    MQTT* mqtt = reinterpret_cast<MQTT*>(param);
    if (mqtt) {
        mqtt->threadEntry();
    }
}

void MQTT::threadEntry() {
    mqtt_client_run(m_client);
}

MQTT::MQTT(const char* clientID, const char* txTopic, const char* rxTopic)
    : Transport(MA_TRANSPORT_MQTT),
      m_client(nullptr),
      m_connected(false),
      m_clientID(clientID),
      m_txTopic(txTopic),
      m_rxTopic(rxTopic) {
    m_client = mqtt_client_new(nullptr);
    MA_ASSERT(m_client);

    mqtt_client_set_id(m_client, m_clientID.c_str());
    mqtt_client_set_userdata(m_client, this);
    mqtt_client_set_callback(m_client, onCallbackStub);

    m_thread = new Thread(clientID, threadEntryStub);
    MA_ASSERT(m_thread);
    m_thread->start(this);
    m_receiveBuffer.assign(MA_MQTT_RECEIVE_BUFFER_SIZE);
}

MQTT::~MQTT() {
    if (m_client) {
        mqtt_client_disconnect(m_client);
        mqtt_client_stop(m_client);
        mqtt_client_free(m_client);
    }
    if (m_thread) {
        delete m_thread;
    }
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

    mqtt_message_t msg;
    msg.topic       = m_txTopic.c_str();
    msg.topic_len   = m_txTopic.length();
    msg.payload     = data;
    msg.payload_len = length;
    msg.qos         = 0;

    int ret = mqtt_client_publish(m_client, &msg);
    return ret == 0 ? length : 0;
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
        if (username && password) {
            mqtt_client_set_auth(m_client, username, password);
        }
        ret = mqtt_client_connect(m_client, host, port, useSSL ? 1 : 0);
    }
    return m_client && ret == 0 ? MA_OK : MA_AGAIN;
}

ma_err_t MQTT::disconnect() {
    Guard guard(m_mutex);
    if (m_client && m_connected.load()) {
        mqtt_client_disconnect(m_client);
        mqtt_client_stop(m_client);
        m_connected.store(false);
    }
    return MA_OK;
}

}  // namespace ma


#endif


