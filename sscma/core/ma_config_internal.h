#ifndef _MA_CONFIG_INTERNAL_H_
#define _MA_CONFIG_INTERNAL_H_

#if __has_include(<ma_config.h>)
    #include <ma_config.h>
#endif

#if __has_include(<ma_config_board.h>)
    #include <ma_config_board.h>
#endif

#ifndef MA_DEBUG_LEVEL
    // Level:
    //      0: no debug
    //      1: print error
    //      2: print warning
    //      3: print info
    //      4: print debug
    //      5: print verbose
    #define MA_DEBUG_LEVEL 5
#endif

#ifndef MA_USE_DEBUG_MORE_INFO
    #define MA_USE_DEBUG_MORE_INFO 0
#endif

#ifndef MA_USE_ASSERT
    #if MA_DEBUG_LEVEL == 0
        #define MA_USE_ASSERT 0
    #else
        #define MA_USE_ASSERT 1
    #endif
#endif

#ifndef MA_ENGINE_SHAPE_MAX_DIM
    #define MA_ENGINE_SHAPE_MAX_DIM 6
#endif

#ifndef MA_MAX_WIFI_SSID_LENGTH
    #define MA_MAX_WIFI_SSID_LENGTH 32
#endif

#ifndef MA_MAX_WIFI_BSSID_LENGTH
    #define MA_MAX_WIFI_BSSID_LENGTH 32
#endif

#ifndef MA_MAX_WIFI_PASSWORD_LENGTH
    #define MA_MAX_WIFI_PASSWORD_LENGTH 64
#endif

#ifndef MA_MQTT_MAX_BROKER_LENGTH
    #define MA_MQTT_MAX_BROKER_LENGTH 128
#endif

#ifndef MA_MQTT_MAX_CLIENT_ID_LENGTH
    #define MA_MQTT_MAX_CLIENT_ID_LENGTH 128
#endif

#ifndef MA_MQTT_MAX_TOPIC_LENGTH
    #define MA_MQTT_MAX_TOPIC_LENGTH 128
#endif

#ifndef MA_MQTT_MAX_USERNAME_LENGTH
    #define MA_MQTT_MAX_USERNAME_LENGTH 128
#endif

#ifndef MA_MQTT_MAX_PASSWORD_LENGTH
    #define MA_MQTT_MAX_PASSWORD_LENGTH 256
#endif

#ifndef MA_MODEL_MAX_PATH_LENGTH
    #define MA_MODEL_MAX_PATH_LENGTH 256
#endif

#ifndef MA_USE_TRANSPORT_MQTT
    #define MA_USE_TRANSPORT_MQTT 0
#endif

#endif  // MA_CONFIG_INTERNAL_H
