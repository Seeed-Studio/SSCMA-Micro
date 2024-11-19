#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

bool isBssid(const std::string& str) {
    if (str.length() != 17) return false;  // BSSID length is 17, e.g. 00:11:22:33:44:55

    for (std::size_t i = 0; i < str.length(); ++i) {
        if (i % 3 == 2) {
            if (str[i] != ':' && str[i] != '-') return false;  // BSSID delimiter is ':' or '-'
        } else {
            if (!std::isxdigit(str[i])) return false;  // BSSID is hex string
        }
    }

    return true;
}

#ifndef MA_HAS_NATTIVE_WIFI_SUPPORT
    #define MA_HAS_NATTIVE_WIFI_SUPPORT 1
#endif

#if MA_USE_EXTERNAL_WIFI_STATUS
extern ma::Mutex    _net_sync_mutex;
extern in4_info_t   _in4_info;
extern in6_info_t   _in6_info;
extern volatile int _net_sta;
#else
static in4_info_t _in4_info{};
static in6_info_t _in6_info{};
#endif

namespace ma::server::callback {

static std::string _wifi_ver    = "";
static int32_t     _wifi_status = 0;

#if !MA_HAS_NATTIVE_WIFI_SUPPORT
void setWifiVer(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    if (argv.size() < 2) {
        ret = MA_EINVAL;
        goto exit;
    }

    _wifi_ver = argv[1];

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("ver", _wifi_ver);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void getWifiVer(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("ver", _wifi_ver);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}
#endif

void configureWifi(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    ma_wifi_config_t config{};
    bool             is_bssid = isBssid(argv[1]);

    if (argv.size() < 4) {
        ret = MA_EINVAL;
        goto exit;
    }

    if (argv[3].length() < 8) {
        ret = MA_EINVAL;
        goto exit;
    }

    if (is_bssid) {
        std::strncpy(config.bssid, argv[1].c_str(), sizeof(config.bssid) - 1);
    } else {
        std::strncpy(config.ssid, argv[1].c_str(), sizeof(config.ssid) - 1);
    }
    std::strncpy(config.password, argv[3].c_str(), sizeof(config.password) - 1);
    config.security = static_cast<ma_wifi_security_t>(std::atoi(argv[2].c_str()));

    MA_STORAGE_SET_ASTR(ret, static_resource->device->getStorage(), MA_STORAGE_KEY_WIFI_SSID, config.ssid);
    MA_STORAGE_SET_ASTR(ret, static_resource->device->getStorage(), MA_STORAGE_KEY_WIFI_BSSID, config.bssid);
    MA_STORAGE_SET_ASTR(ret, static_resource->device->getStorage(), MA_STORAGE_KEY_WIFI_PWD, config.password);
    MA_STORAGE_SET_POD(ret, static_resource->device->getStorage(), MA_STORAGE_KEY_WIFI_SECURITY, config.security);

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write(config);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

#if !MA_HAS_NATTIVE_WIFI_SUPPORT
void setWifiSta(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    if (argv.size() < 2) {
        ret = MA_EINVAL;
        goto exit;
    }

    _wifi_status = std::atoi(argv[1].c_str());

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("status", _wifi_status);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void setWifiIn4Info(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    if (argv.size() < 4) {
        ret = MA_EINVAL;
        goto exit;
    }

    {
    #if MA_USE_EXTERNAL_WIFI_STATUS
        ma::Guard lock(_net_sync_mutex);
    #endif
        _in4_info.ip      = ipv4_addr_t::from_str(argv[1]);
        _in4_info.netmask = ipv4_addr_t::from_str(argv[2]);
        _in4_info.gateway = ipv4_addr_t::from_str(argv[3]);
    }

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    {
    #if MA_USE_EXTERNAL_WIFI_STATUS
        ma::Guard lock(_net_sync_mutex);
    #endif
        encoder.write(_in4_info);
    }
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void setWifiIn6Info(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    if (argv.size() < 2) {
        ret = MA_EINVAL;
        goto exit;
    }

    {
    #if MA_USE_EXTERNAL_WIFI_STATUS
        ma::Guard lock(_net_sync_mutex);
    #endif
        _in6_info.ip = ipv6_addr_t::from_str(argv[1]);
    }
exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    {
    #if MA_USE_EXTERNAL_WIFI_STATUS
        ma::Guard lock(_net_sync_mutex);
    #endif
        encoder.write(_in6_info);
    }
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}
#endif

void getWifiInfo(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    ma_wifi_config_t config{};
    MA_STORAGE_GET_ASTR(static_resource->device->getStorage(), MA_STORAGE_KEY_WIFI_SSID, config.ssid, "");
    MA_STORAGE_GET_ASTR(static_resource->device->getStorage(), MA_STORAGE_KEY_WIFI_BSSID, config.bssid, "");
    MA_STORAGE_GET_ASTR(static_resource->device->getStorage(), MA_STORAGE_KEY_WIFI_PWD, config.password, "");
    MA_STORAGE_GET_POD(static_resource->device->getStorage(), MA_STORAGE_KEY_WIFI_SECURITY, config.security, 0);

    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    {
#if MA_USE_EXTERNAL_WIFI_STATUS
        ma::Guard lock(_net_sync_mutex);
        _wifi_status = _net_sta > 2 ? 2 : _net_sta;
#endif
        if (std::strlen(config.ssid) < 1) {
            _wifi_status = 0;
        }
        encoder.write(_in4_info);
        encoder.write(_in6_info);
    }
    encoder.write(config, (int*)&_wifi_status);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback