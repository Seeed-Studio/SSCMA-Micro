
#ifndef _EL_NETWORK_AT_H
#define _EL_NETWORK_AT_H

#include <hx_drv_uart.h>

#include "el_config_porting.h"
#include "porting/el_network.h"

#define MQTT_CLIENT_ID     "HIMAX_WE2"

#define AT_LONG_TIME_MS    5000
#define AT_SHORT_TIME_MS   1500
#define AT_TX_MAX_LEN      8192  // 默认固件的AT指令长度阈值为256
#define AT_RX_MAX_LEN      8192  // 可能连续收到多条消息

#define AT_STR_HEADER      "AT+"

#define AT_STR_CRLF        "\r\n"
#define AT_STR_ECHO        "ATE0"
#define AT_STR_RST         "RST"
#define AT_STR_CWMODE      "CWMODE"
#define AT_STR_CWJAP       "CWJAP"
#define AT_STR_CWSTATE     "CWSTATE"
#define AT_STR_CWQAP       "CWQAP"

#define AT_STR_MQTTUSERCFG "MQTTUSERCFG"
#define AT_STR_MQTTCONN    "MQTTCONN"
#define AT_STR_MQTTPUB     "MQTTPUB"
#define AT_STR_MQTTSUB     "MQTTSUB"
#define AT_STR_MQTTUNSUB   "MQTTUNSUB"
#define AT_STR_MQTTCLEAN   "MQTTCLEAN"

#define AT_STR_RESP_OK     "OK"
#define AT_STR_RESP_ERROR  "ERROR"
#define AT_STR_RESP_READY  "ready"
#define AT_STR_RESP_WIFI_H "WIFI "
#define AT_STR_RESP_MQTT_H "+MQTT"

#define AT_STR_RESP_PUBRAW ">+MQTTPUB:"

typedef enum {
    AT_CMD_NONE = 0,
    AT_CMD_ECHO,
    AT_CMD_RST,

    AT_CMD_CWMODE,
    AT_CMD_CWJAP,
    AT_CMD_CWSTATE,
    AT_CMD_CWQAP,

    AT_CMD_MQTTUSERCFG,
    AT_CMD_MQTTCONN,
    AT_CMD_MQTTPUB,
    AT_CMD_MQTTSUB,
    AT_CMD_MQTTUNSUB,
    AT_CMD_MQTTCLEAN,
} at_cmd_t;

/*
    BEGIN +------+ INIT +------+ RST +-------+
    ----->| LOST +----->| IDEL +---->| READY |
          +------+      +------+     +---+---+
                                         |
        +----------+    +---------+      |
        |          |<---+         |      |
        | OK/ERROR |    | PROCESS |<-----+
        |          +--->|         |
        +----------+    +---------+
*/
typedef enum {
    AT_STATE_LOST = 0,
    AT_STATE_IDLE,
    AT_STATE_READY,
    AT_STATE_PROCESS,
    AT_STATE_OK,
    AT_STATE_ERROR,
} at_sta_t;

using namespace edgelab;

typedef struct esp_at {
    char tbuf[AT_TX_MAX_LEN];
    // char rbuf[AT_RX_MAX_LEN];
    lwRingBuffer* rbuf;

    DEV_UART_PTR port;
    uint8_t      state;

    topic_cb_t cb;
} esp_at_t;

#endif
