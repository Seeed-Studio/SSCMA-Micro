/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2023 Hongtai Liu (Seeed Technology Inc.)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "el_network_we2.h"

#include <hx_drv_scu.h>

#include "core/el_debug.h"

lwRingBuffer*       at_rbuf        = NULL;
static uint8_t      dma_rx[4]      = {0};
static esp_at_t     at             = {0};
static TaskHandle_t at_rx_parser   = NULL;
static TaskHandle_t status_handler = NULL;

static uint64_t         curr_fail_time       = 0;
static uint64_t         last_fail_time       = 0;
static size_t           fail_count           = 0;
static constexpr size_t fail_count_threshold = 3;
static constexpr size_t fail_time_threshold  = 1000 * 30;

static_assert(fail_time_threshold > AT_LONG_TIME_MS);

static size_t count = 0;

static resp_trigger_t resp_flow[] = {
  {    AT_STR_RESP_OK,     resp_action_ok},
  { AT_STR_RESP_ERROR,  resp_action_error},
  { AT_STR_RESP_READY,  resp_action_ready},
  {AT_STR_RESP_WIFI_H,   resp_action_wifi},
  {AT_STR_RESP_PUBRAW, resp_action_pubraw},
  {AT_STR_RESP_MQTT_H,   resp_action_mqtt},
  {  AT_STR_RESP_IP_H,     resp_action_ip},
  {   AT_STR_RESP_NTP,    resp_action_ntp}
};

static SemaphoreHandle_t at_got_response = NULL;
static SemaphoreHandle_t pubraw_complete = NULL;
static TimerHandle_t     pubraw_tmr      = NULL;
// static uint32_t          last_pub        = 0;

static void pubraw_tmr_cb(TimerHandle_t xTmr) {
    BaseType_t taskwaken = pdFALSE;
    if (at.sent_len >= at.tbuf_len) {  // all tbuf data sent
        // printf("\t[%d] pubraw -> timeout\n", (int)count);

        curr_fail_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (curr_fail_time - last_fail_time > fail_time_threshold) {
            if (fail_count < fail_count_threshold) fail_count = 1;
        } else {
            ++fail_count;
        }
        last_fail_time = curr_fail_time;

        xSemaphoreGiveFromISR(pubraw_complete, &taskwaken);
    }
}

static void dma_tx_cb(void* arg) {
    BaseType_t taskwaken = pdFALSE;
    uint16_t   send_len  = (at.tbuf_len - at.sent_len) > 4095 ? 4095 : (at.tbuf_len - at.sent_len);
    at.sent_len += send_len;
    // EL_LOGD("PUBRAW: %u / %u", at.sent_len, at.tbuf_len);
    if (at.sent_len < at.tbuf_len) {
        send_len = (at.tbuf_len - at.sent_len) > 4095 ? 4095 : (at.tbuf_len - at.sent_len);
        at.port->uart_write_udma(at.tbuf + at.sent_len, send_len, (void*)dma_tx_cb);
    } else {
        // EL_LOGI("AT PUBRAW COMPLETE: %u ms\n", xTaskGetTickCount() - last_pub);
        // last_pub = xTaskGetTickCount();
        xTimerChangePeriodFromISR(pubraw_tmr, AT_LONG_TIME_MS * portTICK_PERIOD_MS, NULL);
    }
}

static el_err_code_t at_send(esp_at_t* at, uint32_t timeout) {
    uint32_t t = 0;
    xSemaphoreTake(at_got_response, 0);
    EL_LOGD("%s\n", at->tbuf);
    at->state = AT_STATE_PROCESS;

    at->port->uart_write(at->tbuf, at->tbuf_len);

    if (xSemaphoreTake(at_got_response, timeout) == pdFALSE) {
        EL_LOGD("AT TIMEOUT\n");
        return EL_ETIMOUT;
    }

    if (at->state != AT_STATE_OK) {
        EL_LOGD("AT STATE ERROR: %d\n", at->state);
        return EL_EIO;
    }
    return EL_OK;
}

static void at_recv_parser(void* arg) {
    char     str[512] = {0};
    char*    ptr      = NULL;
    uint32_t len = 0, i = 0;
    uint32_t num = sizeof(resp_flow) / sizeof(resp_flow[0]);
    while (1) {
        if (ulTaskNotifyTake(pdFALSE, portMAX_DELAY) > 0) {
            len      = at_rbuf->extract('\n', str, sizeof(str));
            str[len] = '\0';
            ptr      = (str[0] == '>') ? str + 1 : str;
            for (i = 0; i < num; i++) {
                if (strncmp(ptr, resp_flow[i].str, strlen(resp_flow[i].str)) == 0) {
                    resp_flow[i].act(ptr, arg);
                    break;
                }
            }
            if (i == num && len > 2) {
                EL_LOGD("unknown response: %s\n", ptr);
            }
            memset(str, 0, len);
        }
    }
}

static void network_status_handler(void* arg) {
    uint32_t             value = 0;
    edgelab::NetworkWE2* net   = (edgelab::NetworkWE2*)arg;
    while (1) {
        if (xTaskNotifyWait(ULONG_MAX, ULONG_MAX, &value, portMAX_DELAY) == pdPASS) {
            // switch (value)
            // {
            // case NETWORK_JOINED:
            //     sprintf(at.tbuf, AT_STR_HEADER AT_STR_CIPSTA "?" AT_STR_CRLF);
            //     at_send(&at, AT_SHORT_TIME_MS);
            // default:
            //     break;
            // }
            net->set_status((el_net_sta_t)value);
        }
    }
}

static void dma_rx_cb(void* arg) {
    BaseType_t taskwaken = pdFALSE;
    if (at.state == AT_STATE_LOST) {
        at.port->uart_read_udma(dma_rx, 1, (void*)dma_rx_cb);
        return;
    }

    *at_rbuf << dma_rx[0];
    if (dma_rx[0] == '\n') {
        vTaskNotifyGiveFromISR(at_rx_parser, &taskwaken);
    }
    at.port->uart_read_udma(dma_rx, 1, (void*)dma_rx_cb);
    portYIELD_FROM_ISR(taskwaken);
}

namespace edgelab {

void NetworkWE2::init(status_cb_t cb) {
    el_err_code_t err = EL_OK;
    _at               = &at;
    at.state          = AT_STATE_LOST;
    if (at_rbuf == NULL) {
        at_rbuf = new lwRingBuffer(AT_RX_MAX_LEN);
        if (at_rbuf == NULL) {
            EL_LOGD("at_rbuf init error\n");
            return;
        }
        at.rbuf = at_rbuf;
    }

    if (at.port == NULL) {
        hx_drv_scu_set_PB6_pinmux(SCU_PB6_PINMUX_UART1_RX, 0);
        hx_drv_scu_set_PB7_pinmux(SCU_PB7_PINMUX_UART1_TX, 0);
        hx_drv_uart_init(USE_DW_UART_1, HX_UART1_BASE);
        at.port = hx_drv_uart_get_dev(USE_DW_UART_1);
        if (at.port == NULL) {
            EL_LOGD("uart init error\n");
            return;
        }
        at.port->uart_open(UART_BAUDRATE_921600);
        memset((void*)at.tbuf, 0, sizeof(at.tbuf));
        at.port->uart_read_udma(dma_rx, 1, (void*)dma_rx_cb);
    }

    // Parse data and trigger events
    if (xTaskCreate(at_recv_parser,
                    "at_recv_parser",
                    CONFIG_EL_NETWORK_STACK_SIZE,
                    this,
                    CONFIG_EL_NETWORK_PRIO,
                    &at_rx_parser) != pdPASS) {
        EL_LOGD("at_recv_parser create error\n");
        return;
    }
    // Handle network status change events
    if (xTaskCreate(network_status_handler,
                    "network_status_handler",
                    CONFIG_EL_NETWORK_STATUS_STACK_SIZE,
                    this,
                    CONFIG_EL_NETWORK_STATUS_PRIO,
                    &status_handler) != pdPASS) {
        EL_LOGD("network_status_handler create error\n");
        return;
    }
    if (at_got_response == NULL) {
        at_got_response = xSemaphoreCreateBinary();
        if (at_got_response == NULL) {
            EL_LOGD("at_got_response create error\n");
            return;
        }
    }
    if (pubraw_complete == NULL) {
        pubraw_complete = xSemaphoreCreateBinary();
        if (pubraw_complete == NULL) {
            EL_LOGD("pubraw_complete create error\n");
            return;
        }
    }
    xSemaphoreGive(pubraw_complete);
    if (pubraw_tmr == NULL) {
        pubraw_tmr = xTimerCreate("pubraw_tmr", pdMS_TO_TICKS(AT_SHORT_TIME_MS), pdFALSE, NULL, pubraw_tmr_cb);
        if (pubraw_tmr == NULL) {
            EL_LOGD("pubraw_tmr create error\n");
            return;
        }
    }
    at.state   = AT_STATE_PROCESS;
    uint32_t t = 0;

    memset((void*)at.tbuf, ' ', 1024);
    memcpy((void*)at.tbuf + 1024, AT_STR_CRLF, strlen(AT_STR_CRLF));
    while (at.state == AT_STATE_PROCESS) {
        at.port->uart_write(at.tbuf, 1024 + strlen(AT_STR_CRLF));
        if (t >= AT_SHORT_TIME_MS) {
            EL_LOGD("AT FLUSH TIMEOUT\n");
            return;
        }
        el_sleep(10);
        t += 10;
    }
    t = 0;
    memset((void*)at.tbuf, 0, 1024 + strlen(AT_STR_CRLF));

    at.tbuf_len = sprintf(at.tbuf, AT_STR_CRLF AT_STR_HEADER AT_STR_RST AT_STR_CRLF);
    at.port->uart_write(at.tbuf, strlen(at.tbuf));
    while (at.state != AT_STATE_READY) {
        if (t >= AT_LONG_TIME_MS) {
            EL_LOGD("AT RST TIMEOUT\n");
            return;
        }
        el_sleep(10);
        t += 10;
    }

    at.tbuf_len = sprintf(at.tbuf, AT_STR_ECHO AT_STR_CRLF);
    err         = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT ECHO ERROR : %d\n", err);
        return;
    }
    at.tbuf_len = sprintf(at.tbuf, AT_STR_HEADER AT_STR_CWMODE "=1" AT_STR_CRLF);
    err         = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT CWMODE ERROR : %d\n", err);
        return;
    }
    if (cb) this->status_cb = cb;
    EL_LOGI("network init ok\n");
    this->_time_synced = false;
    this->set_status(NETWORK_IDLE);
}

el_net_sta_t NetworkWE2::status() {
    if (this->network_status == NETWORK_CONNECTED) {
        if (fail_count >= fail_count_threshold) {
            fail_count = 0;
            this->deinit();
        }
    }
    return this->network_status;
}

void NetworkWE2::set_status(el_net_sta_t status) {
    fail_count = 0;

    this->network_status = status;
    if (this->status_cb) {
        this->status_cb(this->network_status);
    }
}

void NetworkWE2::deinit() {
    // vTaskDelete(status_handler);
    // vTaskDelete(at_rx_parser);

    at.state = AT_STATE_LOST;

    at.tbuf_len = 0;

    // delete at_rbuf;
    // at_rbuf = nullptr;

    this->set_status(NETWORK_LOST);
}

el_err_code_t NetworkWE2::join(const char* ssid, const char* pwd) {
    el_err_code_t err = EL_OK;
    // EL_LOGD("join %s %s\n", ssid, pwd);
    if (this->network_status == NETWORK_JOINED || this->network_status == NETWORK_CONNECTED) {
        return EL_OK;
    } else if (network_status == NETWORK_LOST) {
        return EL_EPERM;
    }
    // AT+CWJAP=[<ssid>],[<pwd>][,<bssid>][,<pci_en>][,<reconn_interval>][,<listen_interval>][,<scan_mode>][,<jap_timeout>][,<pmf>]
    at.tbuf_len = sprintf(at.tbuf, AT_STR_HEADER AT_STR_CWJAP "=\"%s\",\"%s\"" AT_STR_CRLF, ssid, pwd);
    err         = at_send(&at, AT_LONG_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT CWJAP ERROR : %d\n", err);
        return err;
    }
    sprintf(at.tbuf, AT_STR_HEADER AT_STR_CIPSTA "?" AT_STR_CRLF);
    at.tbuf_len = at_send(&at, AT_SHORT_TIME_MS);

    return EL_OK;
}

el_err_code_t NetworkWE2::quit() {
    el_err_code_t err = EL_OK;
    if (this->network_status == NETWORK_IDLE || this->network_status == NETWORK_LOST) {
        return EL_OK;
    }
    at.tbuf_len = sprintf(at.tbuf, AT_STR_HEADER AT_STR_CWQAP AT_STR_CRLF);
    err         = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT CWJAP ERROR : %d\n", err);
        return err;
    }
    return EL_OK;
}

el_err_code_t NetworkWE2::set_mdns(mdns_record_t record) {
    el_err_code_t err = EL_OK;
    at.tbuf_len       = sprintf(at.tbuf, AT_STR_HEADER AT_STR_MDNSSTART "=\"%s\"" AT_STR_CRLF, record.host_name);
    err               = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MDNS ERROR : %d\n", err);
        return err;
    }

    at.tbuf_len =
      sprintf(at.tbuf, AT_STR_HEADER AT_STR_MDNSADD "=\"%s\",\"%s\"" AT_STR_CRLF, MDNS_ITEM_SERVER, record.server);
    err = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MDNS ADD %s ERROR : %d\n", MDNS_ITEM_SERVER, err);
        return err;
    }
    at.tbuf_len =
      sprintf(at.tbuf, AT_STR_HEADER AT_STR_MDNSADD "=\"%s\",\"%d\"" AT_STR_CRLF, MDNS_ITEM_PORT, record.port);
    err = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MDNS ADD %s ERROR : %d\n", MDNS_ITEM_PORT, err);
        return err;
    }
    at.tbuf_len =
      sprintf(at.tbuf, AT_STR_HEADER AT_STR_MDNSADD "=\"%s\",\"%s\"" AT_STR_CRLF, MDNS_ITEM_PROTOCAL, record.protocol);
    err = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MDNS ADD %s ERROR : %d\n", MDNS_ITEM_PROTOCAL, err);
        return err;
    }
    at.tbuf_len =
      sprintf(at.tbuf, AT_STR_HEADER AT_STR_MDNSADD "=\"%s\",\"%s\"" AT_STR_CRLF, MDNS_ITEM_DEST, record.destination);
    err = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MDNS ADD %s ERROR : %d\n", MDNS_ITEM_DEST, err);
        return err;
    }
    at.tbuf_len = sprintf(
      at.tbuf, AT_STR_HEADER AT_STR_MDNSADD "=\"%s\",\"%s\"" AT_STR_CRLF, MDNS_ITEM_AUTH, record.authentication);
    err = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MDNS ADD %s ERROR : %d\n", MDNS_ITEM_AUTH, err);
        return err;
    }

    return EL_OK;
}

el_err_code_t NetworkWE2::connect(const mqtt_server_config_t mqtt_cfg, topic_cb_t cb) {
    el_err_code_t err = EL_OK;
    if (this->network_status == NETWORK_CONNECTED) {
        return EL_OK;
    } else if (this->network_status != NETWORK_JOINED) {
        return EL_EPERM;
    }

    if (cb == NULL) {
        return EL_EINVAL;
    }
    at.cb          = cb;
    this->topic_cb = cb;

    if (mqtt_cfg.use_ssl) {
        if (!this->_time_synced) {
            auto cnt = sprintf(at.tbuf,
                               AT_STR_HEADER AT_STR_CIPSNTPCFG "=1,%d,\"%s\",\"%s\"" AT_STR_CRLF,
                               UTC_TIME_ZONE_CN,
                               SNTP_SERVER_CN,
                               SNTP_SERVER_US);
            EL_LOGI("AT CIPSNTPCFG : %s\n", at.tbuf);
            at.port->uart_write(at.tbuf, cnt);
            uint32_t t = 0;
            while (!this->_time_synced) {
                if (t >= AT_LONG_TIME_MS * 6) {  // 60s
                    EL_LOGI("AT CIPSNTPCFG TIMEOUT\n");
                    return EL_ETIMOUT;
                }
                el_sleep(100);
                t += 100;
            }
        }
        at.tbuf_len = sprintf(at.tbuf, AT_STR_HEADER AT_STR_CIPSNTPTIME AT_STR_CRLF);
        err         = at_send(&at, AT_SHORT_TIME_MS);
        if (err != EL_OK) {
            EL_LOGI("AT CIPSNTPTIME ERROR : %d\n", err);
            return err;
        }
    }

    at.tbuf_len = sprintf(at.tbuf,
                          AT_STR_HEADER AT_STR_MQTTUSERCFG "=0,%d,\"%s\",\"%s\",\"%s\",0,0,\"\"" AT_STR_CRLF,
                          mqtt_cfg.use_ssl ? 4 : 1,
                          mqtt_cfg.client_id,
                          mqtt_cfg.username,
                          mqtt_cfg.password);
    EL_LOGI("AT MQTTUSERCFG : %s\n", at.tbuf);
    err = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGI("AT MQTTUSERCFG ERROR : %d\n", err);
        this->disconnect();
        return err;
    }

    at.tbuf_len =
      sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTCONN "=0,\"%s\",%d,1" AT_STR_CRLF, mqtt_cfg.address, mqtt_cfg.port);
    err = at_send(&at, AT_LONG_TIME_MS * 3);  // 30s
    if (err != EL_OK) {
        EL_LOGI("AT MQTTCONN ERROR : %d\n", err);
        this->disconnect();
        return err;
    }
    this->set_status(NETWORK_CONNECTED);

    return EL_OK;
}

el_err_code_t NetworkWE2::disconnect() {
    el_err_code_t err = EL_OK;
    if (this->network_status != NETWORK_CONNECTED) {
        return EL_EPERM;
    }

    at.tbuf_len = sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTCLEAN "=0" AT_STR_CRLF);
    err         = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MQTTCLEAN ERROR : %d\n", err);
        return err;
    }
    return EL_OK;
}

el_err_code_t NetworkWE2::subscribe(const char* topic, mqtt_qos_t qos) {
    el_err_code_t err = EL_OK;
    if (this->network_status != NETWORK_CONNECTED) {
        return EL_EPERM;
    }

    at.tbuf_len = sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTSUB "=0,\"%s\",%d" AT_STR_CRLF, topic, qos);
    err         = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MQTTSUB ERROR : %d\n", err);
        return err;
    }
    return EL_OK;
}

el_err_code_t NetworkWE2::unsubscribe(const char* topic) {
    el_err_code_t err = EL_OK;
    if (this->network_status != NETWORK_CONNECTED) {
        return EL_EPERM;
    }

    at.tbuf_len = sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTUNSUB "=0,\"%s\"" AT_STR_CRLF, topic);
    err         = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MQTTUNSUB ERROR : %d\n", err);
        return err;
    }
    return EL_OK;
}

el_err_code_t NetworkWE2::publish(const char* topic, const char* dat, uint32_t len, mqtt_qos_t qos) {
    el_err_code_t err = EL_OK;
    if (this->network_status != NETWORK_CONNECTED) {
        return EL_EPERM;
    }

    if (len + strlen(topic) < 200) {
        char    special_chars[] = "\\\"\,\n\r";
        char    buf[230]        = {0};
        uint8_t j               = 0;
        for (uint8_t i = 0; i < len; i++) {
            if (strchr(special_chars, dat[i]) != NULL) {
                buf[j++] = '\\';
            }
            buf[j++] = dat[i];
        }
        buf[j] = '\0';
        at.tbuf_len =
          sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTPUB "=0,\"%s\",\"%s\",%d,0" AT_STR_CRLF, topic, buf, qos);
        err = at_send(&at, AT_SHORT_TIME_MS);
        if (err != EL_OK) {
            EL_LOGD("AT MQTTPUB ERROR : %d\n", err);
            return err;
        }
    } else if (len + 1 < sizeof(at.tbuf)) {
        xSemaphoreTake(pubraw_complete, portMAX_DELAY);
        // last_pub = xTaskGetTickCount();
        uint16_t send_len = len;
        at.tbuf_len =
          sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTPUB "RAW=0,\"%s\",%d,%d,0" AT_STR_CRLF, topic, len, qos);
        err = at_send(&at, AT_SHORT_TIME_MS);
        if (err != EL_OK) {
            // printf("\t[%d] receive_header response timeout\n", (int)count);

            curr_fail_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (curr_fail_time - last_fail_time > fail_time_threshold) {
                if (fail_count < fail_count_threshold) fail_count = 1;
            } else {
                ++fail_count;
            }
            last_fail_time = curr_fail_time;

            xSemaphoreGive(pubraw_complete);
            EL_LOGW("AT MQTTPUB ERROR : %d\n", err);
            return err;
        }

        // snprintf(at.tbuf, len + 1, "%s", dat);
        memcpy(at.tbuf, dat, len);
        at.sent_len = 0;
        at.tbuf_len = len;
        send_len    = at.tbuf_len > 4095 ? 4095 : at.tbuf_len;

        // last_pub = xTaskGetTickCount();
        at.port->uart_write_udma(at.tbuf, send_len, (void*)dma_tx_cb);
    } else {
        EL_LOGD("AT MQTTPUB ERROR : DATA TOO LONG\n");
        return EL_ENOMEM;
    }

    return EL_OK;
}

}  // namespace edgelab

void resp_action_ok(const char* resp, void* arg) {
    EL_LOGD("OK\n");
    at.state = AT_STATE_OK;
    xSemaphoreGive(at_got_response);
}

void resp_action_error(const char* resp, void* arg) {
    EL_LOGD("ERROR\n");
    at.state = AT_STATE_ERROR;

    curr_fail_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (curr_fail_time - last_fail_time > fail_time_threshold) {
        if (fail_count < fail_count_threshold) fail_count = 1;
    } else {
        ++fail_count;
    }
    last_fail_time = curr_fail_time;

    xSemaphoreGive(at_got_response);
}

void resp_action_ready(const char* resp, void* arg) {
    EL_LOGD("READY\n");
    at.state = AT_STATE_READY;
    xSemaphoreGive(at_got_response);
}

void resp_action_wifi(const char* resp, void* arg) {
    if (resp[strlen(AT_STR_RESP_WIFI_H)] == 'G') {
        EL_LOGD("WIFI CONNECTED\n");
        xTaskNotify(status_handler, NETWORK_JOINED, eSetValueWithOverwrite);
    } else if (resp[strlen(AT_STR_RESP_WIFI_H)] == 'D') {
        EL_LOGD("WIFI DISCONNECTED\n");
        xTaskNotify(status_handler, NETWORK_IDLE, eSetValueWithOverwrite);
    }
}

void resp_action_mqtt(const char* resp, void* arg) {
    edgelab::NetworkWE2* net = (edgelab::NetworkWE2*)arg;
    if (resp[strlen(AT_STR_RESP_MQTT_H)] == 'C') {
        EL_LOGD("MQTT CONNECTED\n");
        xTaskNotify(status_handler, NETWORK_CONNECTED, eSetValueWithOverwrite);
    } else if (resp[strlen(AT_STR_RESP_MQTT_H)] == 'D') {
        EL_LOGD("MQTT DISCONNECTED\n");
        xTaskNotify(status_handler, NETWORK_IDLE, eSetValueWithOverwrite);
    } else if (resp[strlen(AT_STR_RESP_MQTT_H)] == 'S') {
        EL_LOGD("MQTT SUBRECV\n");
        int topic_len = 0, msg_len = 0, str_len = 0;

        char* topic_pos = strchr(resp, '"');
        if (topic_pos == NULL) {
            EL_LOGD("MQTT SUBRECV TOPIC ERROR\n");
            return;
        }
        topic_pos++;  // Skip start character
        while (topic_pos[str_len] != '"') {
            str_len++;
        }
        topic_len = str_len;

        str_len       = 0;
        char* msg_pos = topic_pos + topic_len + 1;
        if (msg_pos[0] != ',') {
            EL_LOGD("MQTT SUBRECV MSG ERROR\n");
            return;
        }
        msg_pos++;  // Skip start character
        while (msg_pos[str_len] != ',') {
            str_len++;
        }
        for (int i = 0; i < str_len; i++) {
            if (msg_pos[i] < '0' || msg_pos[i] > '9') {
                EL_LOGD("MQTT SUBRECV MSG ERROR\n");
                return;
            }
            msg_len = msg_len * 10 + (msg_pos[i] - '0');
        }
        msg_pos += str_len + 1;

        if (net->topic_cb) net->topic_cb(topic_pos, topic_len, msg_pos, msg_len);
        return;
    }
}

void resp_action_ip(const char* resp, void* arg) {
    edgelab::NetworkWE2* net = (edgelab::NetworkWE2*)arg;
    uint32_t             ofs = strlen(AT_STR_RESP_IP_H);
    if (strncmp(resp + ofs, "ip:", 3) == 0) {
        ofs += 3;
        net->_ip.ip = ipv4_addr_t::from_str(std::string(resp + ofs, strlen(resp + ofs)));
        return;
    } else if (strncmp(resp + ofs, "gateway:", 8) == 0) {
        ofs += 8;
        net->_ip.gateway = ipv4_addr_t::from_str(std::string(resp + ofs, strlen(resp + ofs)));
        return;
    } else if (strncmp(resp + ofs, "netmask:", 8) == 0) {
        ofs += 8;
        net->_ip.netmask = ipv4_addr_t::from_str(std::string(resp + ofs, strlen(resp + ofs)));
        return;
    }
}

void resp_action_ntp(const char* resp, void* arg) {
    edgelab::NetworkWE2* net = (edgelab::NetworkWE2*)arg;
    EL_LOGD("NTP TIME UPDATED!\n");
    net->_time_synced = true;
    return;
}

void resp_action_pubraw(const char* resp, void* arg) {
    // EL_LOGI("AT PUBRAW RESP: %u ms\n", xTaskGetTickCount() - last_pub);
    // printf("\t[%d] pubraw -> time: %u ms\n", (int)count, xTaskGetTickCount() - last_pub);
    // last_pub = xTaskGetTickCount();
    xTimerStop(pubraw_tmr, 0);
    xSemaphoreGive(pubraw_complete);
}
