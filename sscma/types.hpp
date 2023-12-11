#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace sscma::types {

typedef std::function<void(const std::atomic<bool>&)> repl_task_t;

typedef std::function<void(void*)>                    branch_cb_t;
typedef std::function<int(void*)>                     mutable_cb_t;
typedef std::unordered_map<std::string, mutable_cb_t> mutable_map_t;

struct ipv4_addr_t {
    uint8_t addr[4];

    // we're not going to validate the input string
    static decltype(auto) from_str(std::string s) {
        ipv4_addr_t r{0};
        uint8_t     l{0};

        for (std::size_t i = 0; i < s.length(); ++i) {
            if (!std::isdigit(s[i])) continue;

            uint8_t n{0};
            for (; (++i < s.length()) & (++n < 3);)
                if (!std::isdigit(s[i])) break;
            if (n) {
                auto num{s.substr(i - n, n)};
                r.addr[l++] = static_cast<uint8_t>(std::atoi(num.c_str()));
                if (l > 3) return r;
            }
        }

        return r;
    }

    decltype(auto) to_str() const {
        std::string r{};
        r.reserve(sizeof "255.255.255.255.");
        for (std::size_t i = 0; i < 4; ++i) {
            r += std::to_string(addr[i]);
            r += '.';
        }
        return r.substr(0, r.length() - 1);
    }
};

struct ipv6_addr_t {
    uint16_t addr[8];

    decltype(auto) to_str() const {
        static const char* digits = "0123456789abcdef";
        std::string        r{};
        r.reserve(sizeof "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff:");
        for (std::size_t i = 0; i < 8; ++i) {
            if (addr[i])
                for (uint16_t n = addr[i]; n; n >>= 4) r += digits[n & 0x0f];
            else
                r += '0';
            r += ':';
        }
        return r.substr(0, r.length() - 1);
    }
};

typedef struct in4_info_t {
    ipv4_addr_t ip;
    ipv4_addr_t netmask;
    ipv4_addr_t gateway;
} in4_info_t;

typedef struct in6_info_t {
    ipv6_addr_t ip;
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
