#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <unordered_map>

namespace sscma::types {

typedef std::function<void(const std::atomic<bool>&)> repl_task_t;

typedef std::function<void(void*)>                    branch_cb_t;
typedef std::function<int(void*)>                     mutable_cb_t;
typedef std::unordered_map<std::string, mutable_cb_t> mutable_map_t;

typedef enum wireless_network_name_type_e : uint8_t { SSID, BSSID } wireless_network_name_type_e;

typedef enum wireless_network_secu_type_e : uint8_t {
    AUTO = 0,
    NONE,
    WEP,
    WPA1_WPA2,
    WPA2_WPA3,
    WPA3
} wireless_network_secu_type_e;

typedef struct wireless_network_config_t {
    wireless_network_name_type_e name_type;
    char                         name[SSCMA_WIRELESS_NETWORK_NAME_LEN];
    wireless_network_secu_type_e security_type;
    char                         passwd[SSCMA_WIRELESS_NETWORK_PASSWD_LEN];
} wireless_network_config_t;

typedef struct ipv4_address_t {
    char ip[SSCMA_IPV4_ADDRESS_LEN];
    char netmask[SSCMA_IPV4_ADDRESS_LEN];
    char gateway[SSCMA_IPV4_ADDRESS_LEN];
} ipv4_address_t;

typedef enum mqtt_secu_type_e : uint8_t {
    TCP_NO_CERT = 1,
    TLS_NO_CERT,
    TLS_SERVER_CERT,
    TLS_CLIENT_CERT,
    TLS_BOTH_CERT
} mqtt_secu_type_e;

typedef struct mqtt_server_ssl_config_t {
    char  alpn[SSCMA_MQTT_SSL_ALPN_LEN];
    char* certification_authority;
    char* client_cert;
    char* client_key;
} mqtt_server_ssl_config_t;

typedef struct mqtt_server_config_t {
    char     address[SSCMA_MQTT_ADDRESS_LEN];
    uint32_t port;
    char     client_id[SSCMA_MQTT_CLIENT_ID_LEN];
    char     username[SSCMA_MQTT_USERNAME_LEN];
    char     password[SSCMA_MQTT_PASSWORD_LEN];
    bool     use_ssl;
} mqtt_server_config_t;

typedef struct mqtt_pubsub_config_t {
    char    pub_topic[SSCMA_MQTT_TOPIC_LEN];
    uint8_t pub_qos;
    char    sub_topic[SSCMA_MQTT_TOPIC_LEN];
    uint8_t sub_qos;
} mqtt_pubsub_config_t;

}  // namespace sscma::types
