#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"
#include "resource.hpp"

static int32_t                _mqtt_status = 0;
static ma_mqtt_config_t       _mqtt_server_config{};
static ma_mqtt_topic_config_t _mqtt_topic_config{};

#ifndef MA_HAS_NATTIVE_MQTT_SUPPORT
    #define MA_HAS_NATTIVE_MQTT_SUPPORT 1
#endif

#if MA_USE_EXTERNAL_WIFI_STATUS
extern volatile int _net_sta;
#endif

namespace ma::server::callback {

void getMqttPubsub(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    MA_STORAGE_GET_ASTR(
      static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_PUB_TOPIC, _mqtt_topic_config.pub_topic, "");
    MA_STORAGE_GET_ASTR(
      static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_SUB_TOPIC, _mqtt_topic_config.sub_topic, "");
    MA_STORAGE_GET_POD(
      static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_PUB_QOS, _mqtt_topic_config.pub_qos, 0);
    MA_STORAGE_GET_POD(
      static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_SUB_QOS, _mqtt_topic_config.sub_qos, 0);

    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write(_mqtt_topic_config);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void configureMqttPubsub(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    if (argv.size() < 5) {
        ret = MA_EINVAL;
        goto exit;
    }

    std::strncpy(_mqtt_topic_config.pub_topic, argv[1].c_str(), MA_MQTT_MAX_TOPIC_LENGTH - 1);
    std::strncpy(_mqtt_topic_config.sub_topic, argv[2].c_str(), MA_MQTT_MAX_TOPIC_LENGTH - 1);
    _mqtt_topic_config.pub_qos = std::atoi(argv[3].c_str());
    _mqtt_topic_config.sub_qos = std::atoi(argv[4].c_str());

    MA_STORAGE_SET_ASTR(
      ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_PUB_TOPIC, _mqtt_topic_config.pub_topic);
    MA_STORAGE_SET_ASTR(
      ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_SUB_TOPIC, _mqtt_topic_config.sub_topic);
    MA_STORAGE_SET_POD(
      ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_PUB_QOS, _mqtt_topic_config.pub_qos);
    MA_STORAGE_SET_POD(
      ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_SUB_QOS, _mqtt_topic_config.sub_qos);

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write(_mqtt_topic_config);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void getMqttSta(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
#if MA_USE_EXTERNAL_WIFI_STATUS
    _mqtt_status = (((uint8_t)_net_sta & 0xf) - 1) & 0b0110;
#endif
    encoder.write("status", _mqtt_status);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

#if !MA_HAS_NATTIVE_MQTT_SUPPORT
void setMqttSta(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    if (argv.size() < 2) {
        ret = MA_EINVAL;
        goto exit;
    }

    _mqtt_status = std::atoi(argv[1].c_str());

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("status", _mqtt_status);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}
#endif

void configureMqttServer(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    if (argv.size() < 7) {
        MA_LOGD(MA_TAG, "Invalid number of arguments %d", argv.size());
        ret = MA_EINVAL;
        goto exit;
    }

    std::strncpy(_mqtt_server_config.host, argv[2].c_str(), MA_MQTT_MAX_BROKER_LENGTH - 1);
    _mqtt_server_config.port = std::atoi(argv[3].c_str());
    {
        std::string client_id = argv[1];
        if (client_id.empty()) {
            using namespace std::string_literals;
            client_id = std::string(PRODUCT_NAME_PREFIX) + "_"s + std::string(PRODUCT_NAME_SUFFIX) + "_"s +
                        static_resource->device->id();
        }
        std::strncpy(_mqtt_server_config.client_id, client_id.c_str(), MA_MQTT_MAX_CLIENT_ID_LENGTH - 1);
    }
    std::strncpy(_mqtt_server_config.username, argv[4].c_str(), MA_MQTT_MAX_USERNAME_LENGTH - 1);
    std::strncpy(_mqtt_server_config.password, argv[5].c_str(), MA_MQTT_MAX_PASSWORD_LENGTH - 1);
    _mqtt_server_config.use_ssl = std::atoi(argv[6].c_str());

    MA_STORAGE_SET_ASTR(ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_HOST, _mqtt_server_config.host);
    MA_STORAGE_SET_POD(ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_PORT, _mqtt_server_config.port);
    MA_STORAGE_SET_ASTR(
      ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_CLIENTID, _mqtt_server_config.client_id);
    MA_STORAGE_SET_ASTR(
      ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_USER, _mqtt_server_config.username);
    MA_STORAGE_SET_ASTR(
      ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_PWD, _mqtt_server_config.password);
    MA_STORAGE_SET_POD(
      ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_SSL, _mqtt_server_config.use_ssl);

    {
        using namespace std::string_literals;
        std::string topic_prefix = "sscma/"s + std::string(MA_AT_API_VERSION) + "/"s;
        topic_prefix += _mqtt_server_config.client_id;
        auto tx = topic_prefix + "/tx"s;
        auto rx = topic_prefix + "/rx"s;
        std::strncpy(_mqtt_topic_config.pub_topic, tx.c_str(), MA_MQTT_MAX_TOPIC_LENGTH - 1);
        std::strncpy(_mqtt_topic_config.sub_topic, rx.c_str(), MA_MQTT_MAX_TOPIC_LENGTH - 1);
    }

    MA_STORAGE_SET_ASTR(
      ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_PUB_TOPIC, _mqtt_topic_config.pub_topic);
    MA_STORAGE_SET_ASTR(
      ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_SUB_TOPIC, _mqtt_topic_config.sub_topic);

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write(_mqtt_server_config);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void getMqttConfig(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    MA_STORAGE_GET_ASTR(static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_HOST, _mqtt_server_config.host, "");
    MA_STORAGE_GET_POD(static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_PORT, _mqtt_server_config.port, 0);
    MA_STORAGE_GET_ASTR(
      static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_CLIENTID, _mqtt_server_config.client_id, "");
    MA_STORAGE_GET_ASTR(
      static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_USER, _mqtt_server_config.username, "");
    MA_STORAGE_GET_ASTR(
      static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_PWD, _mqtt_server_config.password, "");
    MA_STORAGE_GET_POD(static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_SSL, _mqtt_server_config.use_ssl, 0);

    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
#if MA_USE_EXTERNAL_WIFI_STATUS
    _mqtt_status = (((uint8_t)_net_sta & 0xf) - 1) & 0b11;
#endif
    if (std::strlen(_mqtt_server_config.host) < 1) {
        _mqtt_status = 0;
    }
    encoder.write(_mqtt_server_config, (int*)&_mqtt_status);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void configureMqttCA(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    std::string ca_string;
    size_t      ca_hash = 0;

    if (argv.size() < 2) {
        ret = MA_EINVAL;
        goto exit;
    }

    ca_string = ma::utils::base64_decode(argv[1]);
    ca_hash   = std::hash<std::string>{}(ca_string);

    MA_STORAGE_SET_STR(ret, static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_SSL_CA, ca_string);

exit:
    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("hash", (uint64_t)ca_hash);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

void getMqttCA(const std::vector<std::string>& argv, Transport& transport, Encoder& encoder) {
    ma_err_t ret = MA_OK;

    std::string b64ca;
    {
        std::string ca_string;
        MA_STORAGE_GET_STR(static_resource->device->getStorage(), MA_STORAGE_KEY_MQTT_SSL_CA, ca_string, "");
        int b64ca_len = ca_string.size() * 4 / 3 + 4;
        b64ca.resize(b64ca_len + 1);
        ma::utils::base64_encode(reinterpret_cast<const unsigned char*>(ca_string.c_str()),
                                 ca_string.size(),
                                 reinterpret_cast<char*>(b64ca.data()),
                                 &b64ca_len);
        b64ca[b64ca_len] = '\0';
    }

    encoder.begin(MA_MSG_TYPE_RESP, ret, argv[0]);
    encoder.write("ca", b64ca);
    encoder.end();
    transport.send(reinterpret_cast<const char*>(encoder.data()), encoder.size());
}

}  // namespace ma::server::callback