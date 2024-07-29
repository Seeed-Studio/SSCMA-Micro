#include <algorithm>
#include <cstring>

#include "ma_camera_mqtt.h"

#if MA_USE_CAMERA_MQTT

/*
 * +-------------------------------------------------------+
 * | 0 | 1 |2-3|4-5| 6 | 7 | 8-9 |10-11 |12-15|16-19|20-23 |
 * +-------------------------------------------------------+
 * |evt|rev|cnt|crc|rot|crc|width|height|size |key |offset|
 * +------------------------------------------------------+
 */

namespace ma {

static constexpr char TAG[] = "ma::camera::mqtt";

void CameraMQTT::onConnect(struct mosquitto* mosq, int rc) {
    MA_LOGD(TAG, "onConnect: %d", rc);
    m_connected.store(true);

    if (rc == MOSQ_ERR_SUCCESS) {
        mosquitto_subscribe(mosq, nullptr, m_rxTopic.c_str(), 0);
    }
}

void CameraMQTT::onDisconnect(struct mosquitto* mosq, int rc) {
    MA_LOGD(TAG, "onDisconnect: %d", rc);
    m_connected.store(false);
}

void CameraMQTT::onPublish(struct mosquitto* mosq, int mid) {
    MA_LOGD(TAG, "onPublish: %d", mid);
}

void CameraMQTT::onSubscribe(struct mosquitto* mosq,
                             int mid,
                             int qos_count,
                             const int* granted_qos) {
    MA_LOGD(TAG, "onSubscribe: %d", mid);
}

void CameraMQTT::onUnsubscribe(struct mosquitto* mosq, int mid) {
    MA_LOGD(TAG, "onUnsubscribe: %d", mid);
}

void CameraMQTT::onMessage(struct mosquitto* mosq, const struct mosquitto_message* msg) {
    MA_LOGD(TAG, "onMessage: %d", msg->mid);
    uint8_t* data           = static_cast<uint8_t*>(msg->payload);
    int length              = msg->payloadlen;
    ma_camera_event_t event = static_cast<ma_camera_event_t>(data[0]);
    uint8_t value           = static_cast<uint8_t>(data[1]);
    MA_LOGD(TAG, "event: %d, value: %d", event, value);
    uint16_t crc    = 0;
    uint16_t cnt    = 0;
    uint32_t key    = 0;
    uint32_t offset = 0;
    ma_img_t* img   = nullptr;
    int shmid       = 0;
    void* shmptr    = nullptr;
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
            key       = ma_ntohl(data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24));
            offset    = ma_ntohl(data[20] | (data[21] << 8) | (data[22] << 16) | (data[23] << 24));

            MA_LOGD(TAG,
                    "idx: %d, offset: %d, count: %d, frame size: %d, width: %d, height: %d, "
                    "format: %d, "
                    "rotate: %d, crc: %d",
                    key,
                    offset,
                    cnt,
                    img->size,
                    img->width,
                    img->height,
                    img->format,
                    img->rotate,
                    crc);

            shmid = shmget(key, 1024, 0666 | IPC_CREAT);

            if (shmid < 0) {
                MA_LOGE(TAG, "shmget failed");
                break;
            }

            shmptr = shmat(shmid, nullptr, 0);
            if (shmptr == (void*)-1) {
                MA_LOGE(TAG, "shmat failed");
                break;
            }

            img->data = static_cast<uint8_t*>(shmptr) + offset;

            if (!m_msgBox.post(img, 0)) {
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

void CameraMQTT::onConnectStub(struct mosquitto* mosq, void* obj, int rc) {
    CameraMQTT* mqtt = static_cast<CameraMQTT*>(obj);
    if (mqtt) {
        mqtt->onConnect(mosq, rc);
    }
}

void CameraMQTT::onDisconnectStub(struct mosquitto* mosq, void* obj, int rc) {

    CameraMQTT* mqtt = static_cast<CameraMQTT*>(obj);
    if (mqtt) {
        mqtt->onDisconnect(mosq, rc);
    }
}

void CameraMQTT::onPublishStub(struct mosquitto* mosq, void* obj, int mid) {

    CameraMQTT* mqtt = static_cast<CameraMQTT*>(obj);
    if (mqtt) {
        mqtt->onPublish(mosq, mid);
    }
}

void CameraMQTT::onSubscribeStub(
    struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos) {

    CameraMQTT* mqtt = static_cast<CameraMQTT*>(obj);
    if (mqtt) {
        mqtt->onSubscribe(mosq, mid, qos_count, granted_qos);
    }
}

void CameraMQTT::onUnsubscribeStub(struct mosquitto* mosq, void* obj, int mid) {

    CameraMQTT* mqtt = static_cast<CameraMQTT*>(obj);
    if (mqtt) {
        mqtt->onUnsubscribe(mosq, mid);
    }
}

void CameraMQTT::onMessageStub(struct mosquitto* mosq,
                               void* obj,
                               const struct mosquitto_message* msg) {

    CameraMQTT* mqtt = static_cast<CameraMQTT*>(obj);
    if (mqtt) {
        mqtt->onMessage(mosq, msg);
    }
}

CameraMQTT::CameraMQTT(const char* clientID, const char* txTopic, const char* rxTopic)
    : m_client(nullptr),
      m_clientID(clientID),
      m_txTopic(txTopic),
      m_rxTopic(rxTopic),
      m_msgBox(30) {

    m_event.clear();
    m_streaming.store(false);
    m_opened.store(false);
    m_connected.store(false);

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
}

CameraMQTT::~CameraMQTT() {

    if (m_client && m_connected.load()) {
        mosquitto_disconnect(m_client);
        mosquitto_loop_stop(m_client, true);
        mosquitto_destroy(m_client);
        m_client = nullptr;
    }

    mosquitto_lib_cleanup();
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
        if (username && password && username[0] && password[0]) {
            mosquitto_username_pw_set(m_client, username, password);
        }
        mosquitto_reconnect_delay_set(m_client, 2, 30, true);
        ret = mosquitto_connect_async(m_client, host, port, 60);
    }

    return m_client && ret == MOSQ_ERR_SUCCESS ? MA_OK : MA_AGAIN;
}

ma_err_t CameraMQTT::disconnect() {
    if (m_client && m_connected.load()) {
        close();
        mosquitto_disconnect(m_client);
        m_connected.store(false);
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

    if (mosquitto_publish(m_client, nullptr, m_txTopic.c_str(), sizeof(cmd), &cmd, 0, false) !=
        MOSQ_ERR_SUCCESS) {
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
    uint32_t value = 0;

    stop();
    ma_camera_event_t cmd = CAM_EVT_CLOSE;
    if (mosquitto_publish(m_client, nullptr, m_txTopic.c_str(), sizeof(cmd), &cmd, 0, false) !=
        MOSQ_ERR_SUCCESS) {
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
    uint32_t value        = 0;

    if (mosquitto_publish(m_client, nullptr, m_txTopic.c_str(), sizeof(cmd), &cmd, 0, false) !=
        MOSQ_ERR_SUCCESS) {
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
    uint32_t value        = 0;
    if (mosquitto_publish(m_client, nullptr, m_txTopic.c_str(), sizeof(cmd), &cmd, 0, false) !=
        MOSQ_ERR_SUCCESS) {
        return MA_AGAIN;
    }
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
    uint32_t value        = 0;

    if (mosquitto_publish(m_client, nullptr, m_txTopic.c_str(), sizeof(cmd), &cmd, 0, false) !=
        MOSQ_ERR_SUCCESS) {
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
    m_msgBox.fetch(reinterpret_cast<void**>(&tmp), timeout);
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


void CameraMQTTProvider::onConnect(struct mosquitto* mosq, int rc) {
    MA_LOGD(TAG, "onConnect: %d", rc);
    m_connected.store(true);
    if (rc == MOSQ_ERR_SUCCESS) {
        mosquitto_subscribe(mosq, nullptr, m_rxTopic.c_str(), 0);
    }
}

void CameraMQTTProvider::onDisconnect(struct mosquitto* mosq, int rc) {
    MA_LOGD(TAG, "onDisconnect: %d", rc);
    m_connected.store(false);
}

void CameraMQTTProvider::onPublish(struct mosquitto* mosq, int mid) {
    MA_LOGD(TAG, "onPublish: %d", mid);
}

void CameraMQTTProvider::onSubscribe(struct mosquitto* mosq,
                                     int mid,
                                     int qos_count,
                                     const int* granted_qos) {
    MA_LOGD(TAG, "onSubscribe: %d", mid);
}

void CameraMQTTProvider::onUnsubscribe(struct mosquitto* mosq, int mid) {
    MA_LOGD(TAG, "onUnsubscribe: %d", mid);
}

void CameraMQTTProvider::onMessage(struct mosquitto* mosq, const struct mosquitto_message* msg) {
    MA_LOGD(TAG, "onMessage: %d", msg->mid);
    uint8_t* data         = static_cast<uint8_t*>(msg->payload);
    ma_camera_event_t evt = static_cast<ma_camera_event_t>(data[0]);
    ma_err_t ret          = MA_OK;
    uint8_t reply[2]      = {evt, 0};
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
        case CAM_EVT_CLEANUP:
            if (ret == MA_OK) {
                m_offset.store(0);
            }
        default:
            break;
    }

    if (mosquitto_publish(mosq, nullptr, m_txTopic.c_str(), 2, reply, 0, false) !=
        MOSQ_ERR_SUCCESS) {
        return;
    }
}

void CameraMQTTProvider::onConnectStub(struct mosquitto* mosq, void* obj, int rc) {
    CameraMQTTProvider* mqtt = static_cast<CameraMQTTProvider*>(obj);
    if (mqtt) {
        mqtt->onConnect(mosq, rc);
    }
}

void CameraMQTTProvider::onDisconnectStub(struct mosquitto* mosq, void* obj, int rc) {

    CameraMQTTProvider* mqtt = static_cast<CameraMQTTProvider*>(obj);
    if (mqtt) {
        mqtt->onDisconnect(mosq, rc);
    }
}

void CameraMQTTProvider::onPublishStub(struct mosquitto* mosq, void* obj, int mid) {

    CameraMQTTProvider* mqtt = static_cast<CameraMQTTProvider*>(obj);
    if (mqtt) {
        mqtt->onPublish(mosq, mid);
    }
}

void CameraMQTTProvider::onSubscribeStub(
    struct mosquitto* mosq, void* obj, int mid, int qos_count, const int* granted_qos) {

    CameraMQTTProvider* mqtt = static_cast<CameraMQTTProvider*>(obj);
    if (mqtt) {
        mqtt->onSubscribe(mosq, mid, qos_count, granted_qos);
    }
}

void CameraMQTTProvider::onUnsubscribeStub(struct mosquitto* mosq, void* obj, int mid) {

    CameraMQTTProvider* mqtt = static_cast<CameraMQTTProvider*>(obj);
    if (mqtt) {
        mqtt->onUnsubscribe(mosq, mid);
    }
}

void CameraMQTTProvider::onMessageStub(struct mosquitto* mosq,
                                       void* obj,
                                       const struct mosquitto_message* msg) {

    CameraMQTTProvider* mqtt = static_cast<CameraMQTTProvider*>(obj);
    if (mqtt) {
        mqtt->onMessage(mosq, msg);
    }
}
CameraMQTTProvider::CameraMQTTProvider(const char* clientID,
                                       const char* txTopic,
                                       const char* rxTopic,
                                       uint32_t shmSize)
    : m_client(nullptr),
      m_clientID(clientID),
      m_txTopic(txTopic),
      m_rxTopic(rxTopic),
      m_shmSize(shmSize),
      m_msgBox(2) {

    m_connected.store(false);
    m_opened.store(false);
    m_streaming.store(false);


    m_shmKey = ftok("/tmp/camera", 65);
    MA_ASSERT(m_shmKey >= 0);
    m_shmId = shmget(m_shmKey, m_shmSize, IPC_CREAT | 0666);
    MA_ASSERT(m_shmId >= 0);
    m_shmAddr = shmat(m_shmId, nullptr, 0);
    MA_ASSERT(m_shmAddr != (void*)-1);
    std::memset(m_shmAddr, 0, m_shmSize);

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
}

CameraMQTTProvider::~CameraMQTTProvider() {
    if (m_client && m_connected.load()) {
        mosquitto_disconnect(m_client);
        mosquitto_loop_stop(m_client, true);
        mosquitto_destroy(m_client);
        m_client = nullptr;
    }
    if (m_shmId >= 0) {
        shmdt(m_shmAddr);
        shmctl(m_shmId, IPC_RMID, nullptr);
    }
    mosquitto_lib_cleanup();
}


ma_err_t CameraMQTTProvider::connect(
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
    uint16_t crc          = 0;
    int ret               = 0;
    uint8_t data[24]      = {0};

    if (img->size + m_offset.load() > m_shmSize) {
        MA_LOGW(TAG, "Discard frame, not enough memory");
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
    reinterpret_cast<uint32_t*>(data + 16)[0] = ma_htonl(m_shmKey);
    reinterpret_cast<uint32_t*>(data + 20)[0] = ma_htonl(m_offset.load());

    std::memcpy(static_cast<char*>(m_shmAddr) + m_offset.load(), img->data, img->size);

    m_offset.store(m_offset.load() + img->size);

    ret = mosquitto_publish(m_client, nullptr, m_txTopic.c_str(), 20, data, 0, false);

    return ret == MOSQ_ERR_SUCCESS ? MA_OK : MA_AGAIN;
}
}  // namespace ma

#endif