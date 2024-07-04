#include <algorithm>
#include <cstring>

#include "ma_camera_mqtt.h"

#if MA_USE_CAMERA_MQTT

/*
 * +------------------------------------------+
 * | 0 | 1 |2-3|4-5| 6 | 7 | 8-9 |10-11 |12-15|
 * +------------------------------------------+
 * |evt|rev|cnt|crc|rot|crc|width|height|size |
 * +------------------------------------------+
 */

namespace ma {

constexpr char TAG[] = "ma::camera::mqtt";

void CameraMQTT::onCallbackStub(mqtt_client_t* cli, int type) {
    CameraMQTT* mqtt = (CameraMQTT*)mqtt_client_get_userdata(cli);
    if (mqtt) {
        mqtt->onCallback(cli, type);
    }
}
void CameraMQTT::prase(const char* data, size_t length) {
    if (length < 2) {
        return;
    }
    ma_camera_event_t event = static_cast<ma_camera_event_t>(data[0]);
    uint8_t value           = static_cast<uint8_t>(data[1]);
    MA_LOGD(TAG, "event: %d, value: %d", event, value);
    uint16_t crc  = 0;
    uint16_t cnt  = 0;
    ma_img_t* img = nullptr;
    switch (event) {
        case CAM_EVT_OPEN:
            m_opened.store(value != 0);
            m_event.set(CAM_EVT_OPEN);
            break;
        case CAM_EVT_CLOSE:
            m_opened.store(value == 0);
            m_event.set(CAM_EVT_CLOSE);
            break;
        case CAM_EVT_START:
            m_streaming.store(value != 0);
            m_event.set(CAM_EVT_START);
            break;
        case CAM_EVT_STOP:
            m_streaming.store(value == 0);
            m_event.set(CAM_EVT_STOP);
            break;
        case CAM_EVT_CAPTURE:
            m_streaming.store(value != 0);
            m_event.set(CAM_EVT_CAPTURE);
            break;
        case CAM_EVT_FRAME:
            if (!m_streaming.load()) {  // not streaming
                break;
            }
            if (length < 12) {
                MA_LOGE(TAG, "length %d < 12", length);
                break;
            }
            img = static_cast<ma_img_t*>(ma_malloc(sizeof(ma_img_t)));
            if (img == nullptr) {
                MA_LOGE(TAG, "malloc failed");
                return;
            }
            cnt         = ma_ntohs(data[2] | (data[3] << 8));
            crc         = ma_ntohs(data[4] | (data[5] << 8));
            img->format = static_cast<ma_pixel_format_t>(data[6]);
            img->rotate = static_cast<ma_pixel_rotate_t>(data[7]);
            img->width  = ma_ntohs(data[8] | (data[9] << 8));
            img->height = ma_ntohs(data[10] | (data[11] << 8));
            img->size = ma_ntohl(data[12] | (data[13] << 8) | (data[14] << 16) | (data[15] << 24));
            img->data = static_cast<uint8_t*>(ma_malloc(img->size));
            if (img->data == nullptr) {
                MA_LOGE(TAG, "malloc failed");
                ma_free(img);
                return;
            }
            memcpy(img->data, data + 16, img->size);
            MA_LOGD(
                TAG,
                "count: %d, frame size: %d, width: %d, height: %d, format: %d, rotate: %d, crc: %d",
                cnt,
                img->size,
                img->width,
                img->height,
                img->format,
                img->rotate,
                crc);
            if (!m_msgBox->post(img, 0)) {
                MA_LOGW(TAG, "discard frame");
                ma_free(img->data);
                ma_free(img);
            }
            break;
        default:
            MA_LOGW(TAG, "unknown event: %d", event);
            break;
    }
}

void CameraMQTT::onCallback(mqtt_client_t* cli, int type) {
    switch (type) {
        case MQTT_TYPE_CONNECT:
            m_connected.store(true);
            break;
        case MQTT_TYPE_CONNACK:
            mqtt_client_subscribe(m_client, m_rxTopic.c_str(), 0);
            break;
        case MQTT_TYPE_DISCONNECT:
            m_connected.store(false);
            m_opened.store(false);
            m_streaming.store(false);
            break;
        case MQTT_TYPE_PUBLISH:
            prase(cli->message.payload, cli->message.payload_len);
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
    mqtt_client_run(m_client);
}

CameraMQTT::CameraMQTT(const char* clientID, const char* txTopic, const char* rxTopic)
    : m_client(nullptr), m_clientID(clientID), m_txTopic(txTopic), m_rxTopic(rxTopic) {

    m_event.clear();
    m_streaming.store(false);
    m_opened.store(false);
    m_connected.store(false);
    m_msgBox = new MessageBox(MA_CAMERA_MQTT_FRAME_QUEUE_SIZE);
    MA_ASSERT(m_msgBox != nullptr);

    m_client = mqtt_client_new(nullptr);
    MA_ASSERT(m_client != nullptr);
    mqtt_client_set_id(m_client, m_clientID.c_str());
    mqtt_client_set_userdata(m_client, this);
    mqtt_client_set_callback(m_client, onCallbackStub);

    m_thread = new Thread(clientID, threadEntryStub);
    MA_ASSERT(m_thread != nullptr);
    m_thread->start(this);
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
    if (m_msgBox) {
        delete m_msgBox;
    }
}

CameraMQTT::operator bool() const {
    return m_connected.load();
}

ma_err_t CameraMQTT::connect(
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

ma_err_t CameraMQTT::disconnect() {
    if (m_client && m_connected.load()) {
        mqtt_client_disconnect(m_client);
        mqtt_client_stop(m_client);
        m_connected.store(false);
        close();
    }
    return MA_OK;
}

ma_err_t CameraMQTT::open() {
    Guard guard(m_mutex);
    if (m_opened.load()) {
        return MA_EBUSY;
    }
    if (!m_client) {
        return MA_EINVAL;
    }
    ma_camera_event_t cmd = CAM_EVT_OPEN;
    mqtt_message_t msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.topic       = m_txTopic.c_str();
    msg.topic_len   = m_txTopic.length();
    msg.payload     = (const char*)(&cmd);
    msg.payload_len = sizeof(cmd);
    msg.qos         = 0;

    if (mqtt_client_publish(m_client, &msg) != 0) {
        return MA_AGAIN;
    }

    uint32_t value;
    m_event.wait(CAM_EVT_OPEN, &value, Tick::fromSeconds(2));
    return m_opened.load() ? MA_OK : MA_AGAIN;
}

ma_err_t CameraMQTT::close() {
    Guard guard(m_mutex);
    if (!m_opened.load()) {
        return MA_EINVAL;
    }
    stop();
    ma_camera_event_t cmd = CAM_EVT_CLOSE;
    mqtt_message_t msg;
    uint32_t value = 0;
    std::memset(&msg, 0, sizeof(msg));
    msg.topic       = m_txTopic.c_str();
    msg.topic_len   = m_txTopic.length();
    msg.payload     = reinterpret_cast<char*>(&cmd);
    msg.payload_len = sizeof(cmd);
    msg.qos         = 0;

    if (mqtt_client_publish(m_client, &msg) != 0) {
        return MA_AGAIN;
    }

    m_event.wait(CAM_EVT_CLOSE, &value, Tick::fromSeconds(2));
    return m_opened.load() ? MA_AGAIN : MA_OK;
}

ma_err_t CameraMQTT::start() {
    Guard guard(m_mutex);
    if (!m_opened.load()) {
        return MA_EINVAL;
    }
    ma_camera_event_t cmd = CAM_EVT_START;
    mqtt_message_t msg;
    uint32_t value = 0;
    std::memset(&msg, 0, sizeof(msg));
    msg.topic       = m_txTopic.c_str();
    msg.topic_len   = m_txTopic.length();
    msg.payload     = reinterpret_cast<char*>(&cmd);
    msg.payload_len = sizeof(cmd);
    msg.qos         = 0;

    if (mqtt_client_publish(m_client, &msg) != 0) {
        return MA_AGAIN;
    }
    m_event.wait(CAM_EVT_START, &value, Tick::fromSeconds(2));
    return m_streaming.load() ? MA_OK : MA_AGAIN;
}

ma_err_t CameraMQTT::stop() {
    Guard guard(m_mutex);
    if (!m_opened.load() || !m_streaming.load()) {
        return MA_EINVAL;
    }
    ma_camera_event_t cmd = CAM_EVT_STOP;
    mqtt_message_t msg;
    uint32_t value = 0;
    std::memset(&msg, 0, sizeof(msg));
    msg.topic       = m_txTopic.c_str();
    msg.topic_len   = m_txTopic.length();
    msg.payload     = reinterpret_cast<char*>(&cmd);
    msg.payload_len = sizeof(cmd);
    msg.qos         = 0;
    mqtt_client_publish(m_client, &msg);
    m_event.wait(CAM_EVT_STOP, &value, Tick::fromSeconds(2));
    return m_streaming.load() ? MA_AGAIN : MA_OK;
}

ma_err_t CameraMQTT::capture() {
    Guard guard(m_mutex);
    if (!m_opened.load()) {
        return MA_EINVAL;
    }
    if (m_streaming.load()) {
        return MA_EBUSY;
    }
    ma_camera_event_t cmd = CAM_EVT_CAPTURE;
    mqtt_message_t msg;
    uint32_t value = 0;
    std::memset(&msg, 0, sizeof(msg));
    msg.topic       = m_txTopic.c_str();
    msg.topic_len   = m_txTopic.length();
    msg.payload     = reinterpret_cast<char*>(&cmd);
    msg.payload_len = sizeof(cmd);
    msg.qos         = 0;

    if (mqtt_client_publish(m_client, &msg) != 0) {
        return MA_AGAIN;
    }

    m_event.wait(CAM_EVT_CAPTURE, &value, Tick::fromSeconds(2));
    return m_streaming.load() ? MA_OK : MA_AGAIN;
}

ma_err_t CameraMQTT::read(ma_img_t* img, ma_tick_t timeout) {
    Guard guard(m_mutex);
    if (!m_opened.load()) {
        return MA_EINVAL;
    }
    ma_img_t* tmp = nullptr;
    m_msgBox->fetch(reinterpret_cast<void**>(&tmp), timeout);
    *img = *tmp;
    delete tmp;
    return MA_OK;
}

ma_err_t CameraMQTT::release(ma_img_t* img) {
    Guard guard(m_mutex);
    if (!m_opened.load()) {
        return MA_EINVAL;
    }
    if (img->data) {
        ma_free(img->data);
    }
    return MA_OK;
}

void CameraMQTTProvider::onCallbackStub(mqtt_client_t* cli, int type) {
    CameraMQTTProvider* mqtt = reinterpret_cast<CameraMQTTProvider*>(mqtt_client_get_userdata(cli));
    if (mqtt) {
        mqtt->onCallback(cli, type);
    }
}

void CameraMQTTProvider::dispatch(const char* data, size_t len) {
    ma_camera_event_t evt = static_cast<ma_camera_event_t>(data[0]);
    ma_err_t ret          = MA_OK;
    uint8_t reply[2]      = {evt, 0};
    mqtt_message_t msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.topic       = m_txTopic.c_str();
    msg.topic_len   = m_txTopic.length();
    msg.payload     = reinterpret_cast<char*>(&reply[0]);
    msg.payload_len = sizeof(reply);
    msg.qos         = 0;
    if (m_callback) {
        ret      = m_callback(evt);
        reply[1] = ret == MA_OK ? 1 : 0;
    }
    switch (evt) {
        case CAM_EVT_OPEN:
            if (ret == MA_OK) {
                m_opened.store(true);
            }
            m_streaming.store(false);
            m_count = 0;
            break;
        case CAM_EVT_CLOSE:
            if (ret == MA_OK) {
                m_opened.store(false);
            }
            m_streaming.store(false);
            break;
        case CAM_EVT_START:
            if (ret == MA_OK) {
                m_streaming.store(true);
            }
            break;
        case CAM_EVT_STOP:
            if (ret == MA_OK) {
                m_streaming.store(false);
            }
            break;
        default:
            break;
    }
    mqtt_client_publish(m_client, &msg);
}

void CameraMQTTProvider::onCallback(mqtt_client_t* cli, int type) {
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
            dispatch(cli->message.payload, cli->message.payload_len);
            break;
        default:
            break;
    }
}

void CameraMQTTProvider::threadEntryStub(void* param) {
    CameraMQTTProvider* mqtt = (CameraMQTTProvider*)param;
    if (mqtt) {
        mqtt->threadEntry();
    }
}

void CameraMQTTProvider::threadEntry() {
    mqtt_client_run(m_client);
}

CameraMQTTProvider::CameraMQTTProvider(const char* clientID,
                                       const char* txTopic,
                                       const char* rxTopic)
    : m_client(nullptr), m_clientID(clientID), m_txTopic(txTopic), m_rxTopic(rxTopic) {

    m_connected.store(false);
    m_opened.store(false);
    m_streaming.store(false);

    m_client = mqtt_client_new(nullptr);
    MA_ASSERT(m_client);
    mqtt_client_set_id(m_client, m_clientID.c_str());
    mqtt_client_set_userdata(m_client, this);
    mqtt_client_set_callback(m_client, onCallbackStub);

    m_thread = new Thread(clientID, threadEntryStub);
    MA_ASSERT(m_thread);

    m_thread->start(this);
}

CameraMQTTProvider::~CameraMQTTProvider() {
    if (m_client) {
        mqtt_client_disconnect(m_client);
        mqtt_client_stop(m_client);
        mqtt_client_free(m_client);
    }
    if (m_thread) {
        delete m_thread;
    }
}


ma_err_t CameraMQTTProvider::connect(
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

CameraMQTTProvider::operator bool() const {
    return m_connected.load();
}

ma_err_t CameraMQTTProvider::write(const ma_img_t* img) {
    Guard guard(m_mutex);
    if (!m_opened.load()) {
        return MA_EINVAL;
    }
    if (!m_streaming.load()) {
        return MA_AGAIN;
    }
    ma_camera_event_t cmd = CAM_EVT_FRAME;
    mqtt_message_t msg;
    uint16_t crc = 0;
    int ret      = 0;
    std::memset(&msg, 0, sizeof(msg));
    uint8_t* data = (uint8_t*)ma_malloc(16 + img->size);
    if (!data) {
        return MA_ENOMEM;
    }

    data[0] = (uint8_t)CAM_EVT_FRAME;
    data[1] = 0;
    data[6] = img->format;
    data[7] = img->rotate;

    reinterpret_cast<uint16_t*>(data + 2)[0]  = ma_htons(m_count++);
    reinterpret_cast<uint16_t*>(data + 4)[0]  = ma_htons(crc);
    reinterpret_cast<uint16_t*>(data + 8)[0]  = ma_htons(img->width);
    reinterpret_cast<uint16_t*>(data + 10)[0] = ma_htons(img->height);
    reinterpret_cast<uint32_t*>(data + 12)[0] = ma_htonl(img->size);

    memcpy(data + 16, img->data, img->size);

    msg.topic       = m_txTopic.c_str();
    msg.topic_len   = m_txTopic.length();
    msg.payload     = reinterpret_cast<char*>(&data[0]);
    msg.payload_len = sizeof(ma_img_t) + img->size;
    msg.qos         = 0;

    ret = mqtt_client_publish(m_client, &msg);

    ma_free(data);
    return ret == 0 ? MA_OK : MA_AGAIN;
}
}  // namespace ma

#endif