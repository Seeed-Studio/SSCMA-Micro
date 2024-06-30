#include <algorithm>
#include <cstring>

#include "ma_camera_mqtt.h"

namespace ma {

const static char* TAG = "ma::camera::mqtt";

void CameraMQTT::onCallbackStub(mqtt_client_t* cli, int type) {
    CameraMQTT* mqtt = (CameraMQTT*)mqtt_client_get_userdata(cli);
    if (mqtt) {
        mqtt->onCallback(cli, type);
    }
}

void CameraMQTT::prase(char* data, size_t length) {
    enum MessageType type = static_cast<enum MessageType>(data[0]);
    ma_img_t img;
    uint16_t crc = 0;
    switch (type) {
        case MSG_TYPE_FRAME:
            img.size   = ma_ntohl(data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24));
            img.width  = ma_ntohs(data[5] | (data[6] << 8));
            img.height = ma_ntohs(data[7] | (data[8] << 8));
            img.format = static_cast<ma_pixel_format_t>(data[9]);
            crc        = ma_ntohs(data[10] | (data[11] << 8));
            img.data   = static_cast<uint8_t*>(ma_malloc(img.size));
            memcpy(img.data, data + 16, img.size);
            m_msgBox->post(&img);
            break;
        case MSG_TYPE_STATUS:
            // TODO
            break;
        default:
            break;
    }
}

void CameraMQTT::onCallback(mqtt_client_t* cli, int type) {
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
            ma_img_t img;
            // img.size = cli->message.payload[0] | (cli->message.payload[1] << 8) |
            //     (cli->message.payload[2] << 16) | (cli->message.payload[3] << 24);
            // img.width  = cli->message.payload[4] | (cli->message.payload[5] << 8);
            // img.height = cli->message.payload[6] | (cli->message.payload[7] << 8);
            // img.format = cli->message.payload[8];
            // img.data   = cli->message.payload + 9;
            m_msgBox->post(&img);
            // TODO consume data
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

void CameraMQTT::threadEntryStub(void* param) {
    CameraMQTT* mqtt = (CameraMQTT*)param;
    if (mqtt) {
        mqtt->threadEntry();
    }
}

void CameraMQTT::threadEntry() {
    MA_LOGD(TAG, "mqtt threadEntry");
    mqtt_client_run(m_client);
}

CameraMQTT::CameraMQTT(const char* clientID, const char* txTopic, const char* rxTopic)
    : m_client(nullptr),
      m_connected(false),
      m_clientID(clientID),
      m_txTopic(txTopic),
      m_rxTopic(rxTopic) {
    m_thread = new Thread(clientID, threadEntryStub);
    if (!m_thread) {
        return;
    }
    m_msgBox = new MessageBox(2);
    if (!m_msgBox) {
        delete m_thread;
        return;
    }
    m_client = mqtt_client_new(nullptr);
    if (m_client) {
        mqtt_client_set_id(m_client, m_clientID.c_str());
        mqtt_client_set_userdata(m_client, this);
        mqtt_client_set_callback(m_client, onCallbackStub);
    }
}

CameraMQTT::~CameraMQTT() {
    if (m_client) {
        mqtt_client_disconnect(m_client);
        mqtt_client_stop(m_client);
        mqtt_client_free(m_client);
    }
    if (m_thread) {
        delete m_thread;
    }
}

CameraMQTT::operator bool() const {
    return m_connected;
}

ma_err_t CameraMQTT::connect(
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

ma_err_t CameraMQTT::disconnect() {
    if (m_client && m_connected) {
        mqtt_client_disconnect(m_client);
        mqtt_client_stop(m_client);
        m_connected = false;
    }
    return MA_OK;
}

ma_err_t CameraMQTT::open() {
    m_opened = true;
    // TODO send open command to MQTT Camera
    return MA_OK;
}

ma_err_t CameraMQTT::close() {
    m_opened = false;
    // TODO send close command to MQTT Camera
    return MA_OK;
}

ma_err_t CameraMQTT::start() {
    // TODO send start command to MQTT Camera
    m_streaming = true;
    return MA_OK;
}

ma_err_t CameraMQTT::stop() {
    // TODO send stop command to MQTT Camera
    m_streaming = false;
    return MA_OK;
}

ma_err_t CameraMQTT::capture() {
    // TODO send capture command to MQTT Camera
    return MA_OK;
}

ma_err_t CameraMQTT::read(ma_img_t* img, int timeout) {
    // TODO read image from MQTT Camera
    m_msgBox->fetch(reinterpret_cast<void**>(&img), timeout);
    return MA_OK;
}

}  // namespace ma
