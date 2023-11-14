#pragma once

#include "core/el_config_internal.h"

#ifndef CONFIG_SSCMA_REPL_EXECUTOR_STACK_SIZE
    #define CONFIG_SSCMA_REPL_EXECUTOR_STACK_SIZE 20480U
#endif

#ifndef CONFIG_SSCMA_REPL_EXECUTOR_PRIO
    #define CONFIG_SSCMA_REPL_EXECUTOR_PRIO 5
#endif

#ifndef CONFIG_SSCMA_REPL_HISTORY_MAX
    #define CONFIG_SSCMA_REPL_HISTORY_MAX 8
#endif

#ifndef CONFIG_SSCMA_TENSOR_ARENA_SIZE
    #define CONFIG_SSCMA_TENSOR_ARENA_SIZE (1024U * 1024U)
#endif

#ifndef CONFIG_SSCMA_CMD_MAX_LENGTH
    #define CONFIG_SSCMA_CMD_MAX_LENGTH 4096
#endif

#define SSCMA_AT_API_MAJOR_VERSION           "v0"

#define SSCMA_EXECUTOR_WORKER_NAME_PREFIX    "sscma#executor"

#define SSCMA_STORAGE_KEY_VERSION            "sscma#version"
#define SSCMA_STORAGE_KEY_ACTION             "sscma#action"
#define SSCMA_STORAGE_KEY_INFO               "sscma#info"
#define SSCMA_STORAGE_KEY_BOOT_COUNT         "sscma#boot_count"
#define SSCMA_STORAGE_KEY_CONF_MODEL_ID      "sscma#conf#model_id"
#define SSCMA_STORAGE_KEY_CONF_SENSOR_ID     "sscma#conf#sensor_id"

#define SSCMA_WIRELESS_NETWORK_NAME_LEN      32
#define SSCMA_WIRELESS_NETWORK_PASSWD_LEN    64
#define SSCMA_WIRELESS_NETWORK_CONN_RETRY    5
#define SSCMA_WIRELESS_NETWORK_POLL_RERTY    5
#define SSCMA_WIRELESS_NETWORK_CONN_DELAY_MS 100

#define SSCMA_MQTT_CONN_RETRY                5
#define SSCMA_MQTT_POLL_RETRY                5
#define SSCMA_MQTT_CONN_DELAY_MS             100
#define SSCMA_MQTT_CLIENT_ID_LEN             32
#define SSCMA_MQTT_ADDRESS_LEN               128
#define SSCMA_MQTT_USERNAME_LEN              64
#define SSCMA_MQTT_PASSWORD_LEN              64
#define SSCMA_MQTT_SSL_ALPN_LEN              SSCMA_MQTT_ADDRESS_LEN
#define SSCMA_MQTT_TOPIC_LEN                 64
#define SSCMA_MQTT_DISCOVER_TOPIC            "sscma/%s/discover"
#define SSCMA_MQTT_DEVICE_ID_FMT             "sscma_%s_%s_%ld"
#define SSCMA_MQTT_PUB_FMT                   "sscma/%s/%s_%s_%ld/tx"
#define SSCMA_MQTT_SUB_FMT                   "sscma/%s/%s_%s_%ld/rx"
