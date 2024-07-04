#ifndef _MA_DEFINATIONS_H_
#define _MA_DEFINATIONS_H_


#define MA_MQTT_CLIENTID_FMT           "%s_%s_%s"
#define MA_MQTT_TOPIC_FMT              "sscma/%s/%s"

#define MA_EXECUTOR_WORKER_NAME_PREFIX "sscma#executor"

#define MA_STORAGE_KEY_ID              "device#id"
#define MA_STORAGE_KEY_NAME            "device#name"

#define MA_STORAGE_DEFAULT_VALUE       "N/A"

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


#endif
