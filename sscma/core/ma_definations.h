#ifndef _MA_DEFINATIONS_H_
#define _MA_DEFINATIONS_H_

#define MA_SSCMA_CONFIG_DEFAULT_PATH   "/etc/sscma.conf"
#define MA_SSCMA_MODEL_DEFAULT_PATH    "/mnt/user/model/"  // TODO change to model dir


#define MA_MQTT_CLIENTID_FMT           "%s_%s"
#define MA_MQTT_TOPIC_FMT              "sscma/v0/%s/%s"

#define MA_EXECUTOR_WORKER_NAME_PREFIX "sscma#executor"

#define MA_STORAGE_KEY_ID              "device#id"
#define MA_STORAGE_KEY_NAME            "device#name"


#define MA_STORAGE_DEFAULT_VALUE       "N/A"

#define MA_STORAGE_KEY_MODEL_ID        "model#id"
#define MA_STORAGE_KEY_MODEL_DIR       "model#addr"

#define MA_STORAGE_KEY_SENSOR_ID       "sensor#id"
#define MA_STORAGE_KEY_SENSOR_OPT_ID   "sensor#opt_id"

#define MA_STORAGE_KEY_WIFI_SSID       "wifi#ssid"
#define MA_STORAGE_KEY_WIFI_BSSID      "wifi#bssid"
#define MA_STORAGE_KEY_WIFI_PWD        "wifi#password"
#define MA_STORAGE_KEY_WIFI_SECURITY   "wifi#security"


#define MA_STORAGE_KEY_MQTT_HOST       "mqtt#host"
#define MA_STORAGE_KEY_MQTT_PORT       "mqtt#port"
#define MA_STORAGE_KEY_MQTT_CLIENTID   "mqtt#client_id"
#define MA_STORAGE_KEY_MQTT_USER       "mqtt#user"
#define MA_STORAGE_KEY_MQTT_PWD        "mqtt#password"
#define MA_STORAGE_KEY_MQTT_PUB_TOPIC  "mqtt#pub_topic"
#define MA_STORAGE_KEY_MQTT_PUB_QOS    "mqtt#pub_qos"
#define MA_STORAGE_KEY_MQTT_SUB_TOPIC  "mqtt#sub_topic"
#define MA_STORAGE_KEY_MQTT_SUB_QOS    "mqtt#sub_qos"
#define MA_STORAGE_KEY_MQTT_SSL        "mqtt#use_ssl"
#define MA_STORAGE_KEY_MQTT_SSL_CA     "mqtt#ssl_ca"

#define MA_STORAGE_KEY_TRIGGER_RULES   "trigger#rules"

#define MA_STORAGE_KEY_WS_PORT           "ws#port"


#define MA_AT_CMD_PREFIX               "AT+"
#define MA_AT_CMD_QUERY                "?"
#define MA_AT_CMD_SET                  "="
#define MA_AT_CMD_SUFFIX               "\r\n"

#define MA_AT_CMD_PREFIX_LEN           (sizeof(MA_AT_CMD_PREFIX) - 1)
#define MA_AT_CMD_SUFFIX_LEN           (sizeof(MA_AT_CMD_SUFFIX) - 1)

#define MA_AT_CMD_WAIT_DELAY           2000  // ms

#define MA_AT_CMD_ID                   "ID"
#define MA_AT_CMD_NAME                 "NAME"
#define MA_AT_CMD_VERSION              "VER"
#define MA_AT_CMD_STATS                "STAT"
#define MA_AT_CMD_BREAK                "BREAK"
#define MA_AT_CMD_RESET                "RST"
#define MA_AT_CMD_WIFI                 "WIFI"
#define MA_AT_CMD_MQTTSERVER           "MQTTSERVER"
#define MA_AT_CMD_MQTTPUBSUB           "MQTTPUBSUB"
#define MA_AT_CMD_INVOKE               "INVOKE"
#define MA_AT_CMD_SAMPLE               "SAMPLE"
#define MA_AT_CMD_INFO                 "INFO"
#define MA_AT_CMD_TSCORE               "TSCORE"
#define MA_AT_CMD_TIOU                 "TIOU"
#define MA_AT_CMD_ALGOS                "ALGOS"
#define MA_AT_CMD_MODELS               "MODELS"
#define MA_AT_CMD_MODEL                "MODEL"
#define MA_AT_CMD_SENSORS              "SENSORS"
#define MA_AT_CMD_SENSOR               "SENSOR"
#define MA_AT_CMD_ACTION               "ACTION"
#define MA_AT_CMD_LED                  "LED"
#define MA_AT_CMD_OTA                  "OTA"

#define MA_AT_EVENT_INVOKE             "INVOKE"
#define MA_AT_EVENT_SAMPLE             "SAMPLE"
#define MA_AT_EVENT_WIFI               "WIFI"
#define MA_AT_EVENT_MQTT               "TransportMQTT"
#define MA_AT_EVENT_SUPERVISOR         "SUPERVISOR"
#define MA_AT_EVENT_INIT               "INIT@STAT"

#define MA_AT_LOG_AT                   "AT"
#define MA_AT_LOG_LOG                  "LOG"


#define MA_NODE_TOPIC_IN_FMT           "sscma/v0/%d/node/in/%s"
#define MA_NODE_TOPIC_OUT_FMT          "sscma/v0/%d/node/out/%s"

#define MA_NODE_CMD_CLEAR              "clear"
#define MA_NODE_CMD_CREATE             "create"
#define MA_NODE_CMD_DESTROY            "destroy"
#define MA_NODE_CMD_CONFIG             "config"
#define MA_NODE_CMD_STREAM             "stream"
#define MA_NODE_CMD_INVOKE             "invoke"

#endif
