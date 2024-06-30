#include <algorithm>
#include <cstring>

#include "ma_transport_mqtt.h"

#ifdef MA_USE_TRANSPORT_MQTT

namespace ma {

const static char* TAG = "ma::transport::mqtt";

void MQTT::onCallbackStub(mqtt_client_t* cli, int type) {
    MQTT* mqtt = (MQTT*)mqtt_client_get_userdata(cli);
    if (mqtt) {
        mqtt->onCallback(cli, type);
    }
}

void MQTT::onCallback(mqtt_client_t* cli, int type) {
    MA_LOGD(TAG, "mqtt onCallback %d", type);
    switch (type) {
        case MQTT_TYPE_CONNECT:
            MA_LOGI(TAG, "mqtt connected!");
            m_connected = true;
            break;
        case MQTT_TYPE_CONNACK:
            mqtt_client_subscribe(m_client, m_rxTopic.c_str(), 0);
            break;
        case MQTT_TYPE_DISCONNECT:
            MA_LOGI(TAG, "mqtt disconnected!");
            m_connected = false;
            break;
        case MQTT_TYPE_PUBLISH:
            MA_LOGI(TAG, "mqtt publish: %d", cli->message.payload_len);
            m_receiveBuffer.write(cli->message.payload, cli->message.payload_len);
            break;
        case MQTT_TYPE_PUBACK:
            MA_LOGI(TAG, "mqtt puback!");
            break;
        case MQTT_TYPE_PUBREC:
            MA_LOGI(TAG, "mqtt pubrec!");
            break;
        case MQTT_TYPE_PUBREL:
            MA_LOGI(TAG, "mqtt pubrel!");
            break;
        case MQTT_TYPE_PUBCOMP:
            MA_LOGI(TAG, "mqtt pubcomp!");
            break;
        default:
            break;
    }
}

void MQTT::threadEntryStub(void* param) {
    MQTT* mqtt = (MQTT*)param;
    if (mqtt) {
        mqtt->threadEntry();
    }
}

void MQTT::threadEntry() {
    MA_LOGD(TAG, "mqtt threadEntry");
    mqtt_client_run(m_client);
}

MQTT::MQTT(const char* clientID, const char* txTopic, const char* rxTopic)
    : Transport(MA_TRANSPORT_MQTT),
      m_client(nullptr),
      m_connected(false),
      m_clientID(clientID),
      m_txTopic(txTopic),
      m_rxTopic(rxTopic) {
    m_thread = new Thread(clientID, threadEntryStub);
    if (!m_thread) {
        return;
    }
    m_client = mqtt_client_new(nullptr);
    if (m_client) {
        mqtt_client_set_id(m_client, m_clientID.c_str());
        mqtt_client_set_userdata(m_client, this);
        mqtt_client_set_callback(m_client, onCallbackStub);
    }
    m_receiveBuffer.assign(1024);
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
    return m_client;
}

size_t MQTT::available() const {
    return m_receiveBuffer.size();
}

size_t MQTT::send(const char* data, size_t length, int timeout) {
    if (!m_client || !m_connected) {
        return 0;
    }

    mqtt_message_t msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.topic       = m_txTopic.c_str();
    msg.topic_len   = m_txTopic.length();
    msg.payload     = (char*)data;
    msg.payload_len = length;
    msg.qos         = 0;

    int ret = mqtt_client_publish(m_client, &msg);
    return ret == 0 ? length : 0;
}

size_t MQTT::receive(char* data, size_t length, int timeout) {
    if (m_receiveBuffer.empty()) {
        return 0;
    }
    size_t ret = m_receiveBuffer.read(data, length);
    return ret;
}

size_t MQTT::receiveUtil(char* data, size_t length, char delimiter, int timeout) {
    if (m_receiveBuffer.empty()) {
        return 0;
    }
    return m_receiveBuffer.readUtil(data, length, delimiter);
}

ma_err_t MQTT::connect(
    const char* host, int port, const char* username, const char* password, bool useSSL) {
    if (m_connected) {
        return MA_EBUSY;
    }
    if (m_client) {
        if (username && password) {
            mqtt_client_set_auth(m_client, username, password);
        }
        int ret     = mqtt_client_connect(m_client, host, port, useSSL ? 1 : 0);
        m_connected = (ret == 0);
    }
    if (m_connected) {
        m_thread->start(this);
    }
    return m_client && m_connected ? MA_OK : MA_AGAIN;
}

ma_err_t MQTT::disconnect() {
    if (m_client && m_connected) {
        mqtt_client_disconnect(m_client);
        mqtt_client_stop(m_client);
        m_connected = false;
    }
    return MA_OK;
}

}  // namespace ma


#endif
