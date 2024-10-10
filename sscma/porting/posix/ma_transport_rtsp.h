#ifndef _MA_TRANSPORT_RTSP_H
#define _MA_TRANSPORT_RTSP_H


#include "core/ma_common.h"

#if MA_USE_TRANSPORT_RTSP

#include <BasicUsageEnvironment.hh>
#include <liveMedia.hh>
#include <rtsp.h>


#include "porting/ma_osal.h"
#include "porting/ma_transport.h"


namespace ma {

class TransportRTSP final : public Transport {


public:
    TransportRTSP();
    ~TransportRTSP();


    struct RTSPConfig {
        int port;
        ma_pixel_format_t format;
        std::string session;
        std::string user;
        std::string pass;
    };

    ma_err_t init(const void* config) noexcept override;
    void deInit() noexcept override;


    size_t available() const noexcept override;
    size_t send(const char* data, size_t length) noexcept override;
    size_t receive(char* data, size_t length) noexcept override;
    size_t receiveIf(char* data, size_t length, char delimiter) noexcept override;
    size_t flush() noexcept override;

private:
    ma_pixel_format_t m_format;
    int m_port;
    std::string m_name;
    std::string m_user;
    std::string m_pass;
    CVI_RTSP_SESSION* m_session;
    CVI_RTSP_CTX* m_ctx;
    Mutex m_mutex;
    std::vector<std::string> m_ip_list;
    static std::unordered_map<int, CVI_RTSP_CTX*> s_contexts;
    static std::unordered_map<int, UserAuthenticationDatabase*> s_auths;
    static std::unordered_map<int, std::vector<CVI_RTSP_SESSION*>> s_sessions;
};

}  // namespace ma

#endif

#endif