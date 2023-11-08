#pragma once

#include "core/el_config_internal.h"

#ifndef CONFIG_SSCMA_REPL_EXECUTOR_STACK_SIZE
    #define CONFIG_SSCMA_REPL_EXECUTOR_STACK_SIZE (20480)
#endif

#ifndef CONFIG_SSCMA_REPL_EXECUTOR_PRIO
    #define CONFIG_SSCMA_REPL_EXECUTOR_PRIO (5)
#endif

#ifndef CONFIG_SSCMA_REPL_HISTORY_MAX
    #define CONFIG_SSCMA_REPL_HISTORY_MAX (8)
#endif

#ifndef CONFIG_SSCMA_TENSOR_ARENA_SIZE
    #define CONFIG_SSCMA_TENSOR_ARENA_SIZE (1024 * 1024)
#endif

#ifndef CONFIG_SSCMA_CMD_MAX_LENGTH
    #define CONFIG_SSCMA_CMD_MAX_LENGTH (4096)
#endif

#define SSCMA_STORAGE_KEY_VERSION            "sscma#core#version"
#define SSCMA_STORAGE_KEY_ACTION             "sscma#action"
#define SSCMA_STORAGE_KEY_INFO               "sscma#info"

#define SSCMA_WIRELESS_NETWORK_NAME_LEN      32
#define SSCMA_WIRELESS_NETWORK_PASSWD_LEN    64
#define SSCMA_WIRELESS_NETWORK_CONN_RETRY    5
#define SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS 100

#define SSCMA_MQTT_CONN_RETRY                5
#define SSCMA_MQTT_CONN_DELAY_MS             100
#define SSCMA_MQTT_CLIENT_ID_LEN             32
#define SSCMA_MQTT_ADDRESS_LEN               128
#define SSCMA_MQTT_USERNAME_LEN              64
#define SSCMA_MQTT_PASSWORD_LEN              64
#define SSCMA_MQTT_SSL_ALPN_LEN              SSCMA_MQTT_ADDRESS_LEN
#define SSCMA_MQTT_TOPIC_LEN                 64
