#include "ma_transport_rtsp.h"

#if MA_USE_TRANSPORT_RTSP

namespace ma {

static const char* TAG = "ma::transport::rtsp";

std::unordered_map<int, CVI_RTSP_CTX*> TransportRTSP::s_contexts;
std::unordered_map<int, UserAuthenticationDatabase*> TransportRTSP::s_auths;
std::unordered_map<int, std::vector<CVI_RTSP_SESSION*>> TransportRTSP::s_sessions;

TransportRTSP::TransportRTSP() : Transport(MA_TRANSPORT_RTSP), m_format(MA_PIXEL_FORMAT_H264), m_port(0), m_session(nullptr), m_ctx(nullptr) {}

TransportRTSP::~TransportRTSP() {
    deInit();
}

static void onConnectStub(const char* ip, void* arg) {
    MA_LOGD(TAG, "rtsp connected: %s", ip);
}

static void onDisconnectStub(const char* ip, void* arg) {
    MA_LOGD(TAG, "rtsp disconnected: %s", ip);
}

ma_err_t TransportRTSP::init(const void* config) noexcept {
    ma_err_t ret;
    Guard guard(m_mutex);

    // only support H264
    if (m_format != MA_PIXEL_FORMAT_H264) {
        return MA_ENOTSUP;
    }

    RTSPConfig* rtsp_config = (RTSPConfig*)config;
    m_port                  = rtsp_config->port;
    m_format                = rtsp_config->format;
    m_name                  = rtsp_config->session;

    if (m_initialized) {
        return MA_EBUSY;
    }

    m_user = rtsp_config->user;
    m_pass = rtsp_config->pass;

    if (s_contexts.find(m_port) == s_contexts.end()) {
        CVI_RTSP_CONFIG config = {0};
        config.port            = m_port;
        if (CVI_RTSP_Create(&m_ctx, &config) < 0) {
            return MA_EINVAL;
        }
        s_contexts[m_port] = m_ctx;
        if (CVI_RTSP_Start(m_ctx) < 0) {
            CVI_RTSP_Destroy(&m_ctx);
            return MA_EINVAL;
        }
        CVI_RTSP_STATE_LISTENER listener = {0};
        listener.onConnect               = onConnectStub;
        listener.argConn                 = this;
        listener.onDisconnect            = onDisconnectStub;
        listener.argDisconn              = this;

        CVI_RTSP_SetListener(m_ctx, &listener);

        RTSPServer* server               = static_cast<RTSPServer*>(m_ctx->server);
        UserAuthenticationDatabase* auth = new UserAuthenticationDatabase();
        if (auth == nullptr) {
            CVI_RTSP_Destroy(&m_ctx);
            return MA_ENOMEM;
        }
        server->setAuthenticationDatabase(auth);
        s_auths[m_port] = auth;
        MA_LOGV(TAG, "rtsp sever created: %d", m_port);
    } else {
        m_ctx = s_contexts[m_port];
    }

    if (m_user.size() > 0 && m_pass.size() > 0) {
        MA_LOGI(TAG, "rtsp user: %s pass: %s", m_user.c_str(), m_pass.c_str());
        s_auths[m_port]->addUserRecord(m_user.c_str(), m_pass.c_str());
    }

    for (auto& session : s_sessions[m_port]) {
        if (session->name == m_name) {
            MA_LOGW(TAG, "rtsp session already exists: %d/%s", m_port, m_name.c_str());
            return MA_EBUSY;
        }
    }
    CVI_RTSP_SESSION_ATTR attr = {0};

    attr.reuseFirstSource = true;

    switch (m_format) {
        case MA_PIXEL_FORMAT_H264:
            attr.video.codec = RTSP_VIDEO_H264;
            break;
        case MA_PIXEL_FORMAT_H265:
            attr.video.codec = RTSP_VIDEO_H265;
            break;
        case MA_PIXEL_FORMAT_JPEG:
            attr.video.codec = RTSP_VIDEO_JPEG;
            break;
        default:
            return MA_EINVAL;
    }

    snprintf(attr.name, sizeof(attr.name), "%s", m_name.c_str());

    CVI_RTSP_CreateSession(m_ctx, &attr, &m_session);

    s_sessions[m_port].push_back(m_session);

    MA_LOGI(TAG, "rtsp session created: %d/%s", m_port, m_name.c_str());

    m_initialized = true;

    return MA_OK;
}


void TransportRTSP::deInit() noexcept {
    Guard guard(m_mutex);
    if (!m_initialized) {
        return;
    }
    s_auths[m_port]->removeUserRecord(m_user.c_str());
    CVI_RTSP_DestroySession(m_ctx, m_session);

    MA_LOGI(TAG, "rtsp session destroyed: %d/%s", m_port, m_name.c_str());

    auto it = std::find(s_sessions[m_port].begin(), s_sessions[m_port].end(), m_session);
    if (it != s_sessions[m_port].end()) {
        s_sessions[m_port].erase(it);
    }

    if (s_sessions[m_port].empty()) {
        MA_LOGV(TAG, "rtsp server destroy: %d", m_port);
        RTSPServer* server = static_cast<RTSPServer*>(m_ctx->server);
        auto* old          = server->setAuthenticationDatabase(nullptr);
        if (old) {
            delete old;
        }

        CVI_RTSP_Stop(m_ctx);
        CVI_RTSP_Destroy(&m_ctx);
        s_contexts.erase(m_port);
    }

    m_initialized = false;
    return;
}


size_t TransportRTSP::available() const noexcept {
    return 0;
}


size_t TransportRTSP::flush() noexcept {
    return 0;
}


size_t TransportRTSP::send(const char* data, size_t length) noexcept {
    Guard guard(m_mutex);
    if (!m_initialized) {
        return 0;
    }
    CVI_RTSP_DATA frame = {0};
    frame.blockCnt      = 1;
    frame.dataPtr[0]    = reinterpret_cast<uint8_t*>(const_cast<char*>(data));
    frame.dataLen[0]    = length;

    CVI_RTSP_WriteFrame(s_contexts[m_port], m_session->video, (CVI_RTSP_DATA*)&frame);

    return 0;
}


size_t TransportRTSP::receive(char* data, size_t length) noexcept {
    return 0;
}


size_t TransportRTSP::receiveIf(char* data, size_t length, char delimiter) noexcept {
    return 0;
}


}  // namespace ma

#endif