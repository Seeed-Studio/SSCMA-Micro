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

typedef struct ipv4_addr_t {
    uint8_t addr[4];
} ipv4_addr_t;

typedef struct ipv6_addr_t {
    uint16_t addr[8];
} ipv6_addr_t;

typedef struct in4_info_t {
    ipv4_addr_t ip;
    ipv4_addr_t netmask;
    ipv4_addr_t gateway;
    ipv4_addr_t dns_server;
} in4_info_t;

typedef struct in6_info_t {
    ipv6_addr_t ip;
    ipv6_addr_t netmask;
    ipv6_addr_t gateway;
    ipv6_addr_t dns_server;
} in6_info_t;

typedef enum wifi_name_type_e : uint8_t { SSID, BSSID } wifi_name_type_e;

typedef enum wifi_secu_type_e : uint8_t { AUTO = 0, NONE, WEP, WPA1_WPA2, WPA2_WPA3, WPA3 } wifi_secu_type_e;

typedef struct wifi_config_t {
    wifi_name_type_e name_type;
    char             name[SSCMA_WIFI_NAME_LEN];
    wifi_secu_type_e security_type;
    char             passwd[SSCMA_WIFI_PASSWD_LEN];
} wifi_config_t;

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
