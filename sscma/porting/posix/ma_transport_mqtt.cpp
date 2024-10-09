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
        mosquitto_subscribe(mosq, nullptr, m_topicConfig.sub_topic, m_topicConfig.sub_qos);
    }
}

void TransportMQTT::onDisconnect(struct mosquitto* mosq, int rc) {
    m_connected.store(false);
}

void TransportMQTT::onMessage(struct mosquitto* mosq, const struct mosquitto_message* msg) {
    m_receiveBuffer->push(static_cast<const char*>(msg->payload), msg->payloadlen);
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
void TransportMQTT::onMessageStub(struct mosquitto* mosq, void* obj, const struct mosquitto_message* msg) {

    TransportMQTT* mqtt = static_cast<TransportMQTT*>(obj);
    if (mqtt) {
        mqtt->onMessage(mosq, msg);
    }
}


TransportMQTT::TransportMQTT(ma_mqtt_config_t* config) : Transport(MA_TRANSPORT_MQTT), m_mutex(), m_client(nullptr), m_connected(false) {

    MA_ASSERT(config != nullptr);

    memset(&m_config, 0, sizeof(m_config));
    memset(&m_topicConfig, 0, sizeof(m_topicConfig));

    m_config = *config;

    mosquitto_lib_init();

    m_client = mosquitto_new(config->client_id, true, this);
    MA_ASSERT(m_client);

    mosquitto_connect_callback_set(m_client, onConnectStub);
    mosquitto_disconnect_callback_set(m_client, onDisconnectStub);
    mosquitto_message_callback_set(m_client, onMessageStub);

    mosquitto_loop_start(m_client);

    m_receiveBuffer = new SPSCRingBuffer<char>(MA_MQTT_RECEIVE_BUFFER_SIZE);
}

TransportMQTT::~TransportMQTT() noexcept {

    if (m_client) {
        mosquitto_loop_stop(m_client, true);
        mosquitto_destroy(m_client);
    }

    mosquitto_lib_cleanup();
}

size_t TransportMQTT::available() const noexcept {
    Guard guard(m_mutex);
    return m_receiveBuffer->size();
}

size_t TransportMQTT::send(const char* data, size_t length) noexcept {
    Guard guard(m_mutex);
    if (!m_client || !m_connected.load()) {
        return 0;
    }
    int ret = mosquitto_publish(m_client, nullptr, m_topicConfig.pub_topic, length, data, m_topicConfig.pub_qos, false);
    return ret == MOSQ_ERR_SUCCESS ? length : 0;
}

size_t TransportMQTT::receive(char* data, size_t length) noexcept {
    Guard guard(m_mutex);
    if (m_receiveBuffer->empty()) {
        return 0;
    }
    size_t ret = m_receiveBuffer->pop(data, length);
    return ret;
}

size_t TransportMQTT::receiveIf(char* data, size_t length, char delimiter) noexcept {
    Guard guard(m_mutex);
    if (m_receiveBuffer->empty()) {
        return 0;
    }
    return m_receiveBuffer->popIf(data, length, delimiter);
}

ma_err_t TransportMQTT::init(const void* config) noexcept {
    Guard guard(m_mutex);
    int ret = 0;
    if (m_initialized) [[unlikely]] {
        return MA_OK;
    }

    if (!config) [[unlikely]] {
        return MA_EINVAL;
    }

    if (m_connected.load()) {
        return MA_EBUSY;
    }

    m_topicConfig = *(ma_mqtt_topic_config_t*)config;

    MA_LOGD(TAG, "connect to mqtt://%s:%d", m_config.host, m_config.port);
    MA_LOGD(TAG, "topic_in: %s, topic_out: %s", m_topicConfig.sub_topic, m_topicConfig.pub_topic);


    if (m_client) {
        if (m_config.username[0] != '\0' && m_config.password[0] != '\0') {
            mosquitto_username_pw_set(m_client, m_config.username, m_config.password);
        }
        mosquitto_reconnect_delay_set(m_client, 2, 30, true);
        ret = mosquitto_connect_async(m_client, m_config.host, m_config.port, 60);
    }
    m_initialized = true;
    return m_client && ret == MOSQ_ERR_SUCCESS ? MA_OK : MA_AGAIN;
}


void TransportMQTT::deInit() noexcept {
    Guard guard(m_mutex);
    if (!m_initialized) [[unlikely]] {
        return;
    }

    if (m_connected.load()) {
        mosquitto_disconnect(m_client);
        m_connected.store(false);
    }

    m_initialized = false;

    return;
}

size_t TransportMQTT::flush() noexcept {
    return 0;
}

}  // namespace ma


#endif
