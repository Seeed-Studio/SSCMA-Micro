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

lwRingBuffer*       at_rbuf   = NULL;
static uint8_t      dma_rx[4] = {0};
static esp_at_t     at;
static TaskHandle_t at_rx_parser   = NULL;
static TaskHandle_t status_handler = NULL;

static el_err_code_t at_send(esp_at_t* at, uint32_t timeout) {
    uint32_t t = 0;
    // EL_LOGI("%s\n", at->tbuf);
    at->state = AT_STATE_PROCESS;
    at->port->uart_write(at->tbuf, strlen(at->tbuf));

    while (at->state == AT_STATE_PROCESS) {
        if (t >= timeout) {
            // EL_LOGD("AT TIMEOUT\n");
            return EL_ETIMOUT;
        }
        el_sleep(10);
        t += 10;
    }

    if (at->state != AT_STATE_OK) {
        EL_LOGD("AT STATE ERROR: %d\n", at->state);
        return EL_EIO;
    }
    return EL_OK;
}

static void newline_parse(topic_cb_t cb) {
    char     str[512] = {0};
    uint32_t len      = at_rbuf->extract('\n', str, sizeof(str));

    // command response : OK or ERROR
    if (len < 3) return;
    if (strncmp(str, AT_STR_RESP_OK, strlen(AT_STR_RESP_OK)) == 0) {
        // EL_LOGD("OK\n");
        at.state = AT_STATE_OK;
        return;
    } else if (strncmp(str, AT_STR_RESP_ERROR, strlen(AT_STR_RESP_ERROR)) == 0) {
        // EL_LOGD("ERROR\n");
        at.state = AT_STATE_ERROR;
        return;
    }

    // status change or data receive
    if (len < 6) return;
    if (strncmp(str, AT_STR_RESP_READY, strlen(AT_STR_RESP_READY)) == 0) {
        EL_LOGD("READY\n");
        at.state = AT_STATE_READY;
        return;
    } else if (strncmp(str, AT_STR_RESP_WIFI_H, strlen(AT_STR_RESP_WIFI_H)) == 0) {
        if (str[strlen(AT_STR_RESP_WIFI_H)] == 'C') {
            EL_LOGD("WIFI CONNECTED\n");
            xTaskNotify(status_handler, NETWORK_JOINED, eSetValueWithOverwrite);
            return;
        } else if (str[strlen(AT_STR_RESP_WIFI_H)] == 'D') {
            EL_LOGD("WIFI DISCONNECTED\n");
            xTaskNotify(status_handler, NETWORK_IDLE, eSetValueWithOverwrite);
            return;
        }
    } else if (strncmp(str, AT_STR_RESP_MQTT_H, strlen(AT_STR_RESP_MQTT_H)) == 0) {
        if (str[strlen(AT_STR_RESP_MQTT_H)] == 'C') {
            EL_LOGD("MQTT CONNECTED\n");
            xTaskNotify(status_handler, NETWORK_CONNECTED, eSetValueWithOverwrite);
            return;
        } else if (str[strlen(AT_STR_RESP_MQTT_H)] == 'D') {
            EL_LOGD("MQTT DISCONNECTED\n");
            xTaskNotify(status_handler, NETWORK_IDLE, eSetValueWithOverwrite);
            return;
        } else if (str[strlen(AT_STR_RESP_MQTT_H)] == 'S') {  // subscribe received
            EL_LOGD("MQTT SUBRECV\n");
            // EL_LOGD("%s\n", str);
            // EXAMPLE: +MQTTSUBRECV:0,"topic",4,test
            int topic_len = 0, msg_len = 0, str_len = 0;

            char* topic_pos = strchr(str, '"');
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

            if (cb) cb(topic_pos, topic_len, msg_pos, msg_len);
            return;
        }
    }
}

static void at_recv_parser(void* arg) {
    edgelab::NetworkWE2* net = (edgelab::NetworkWE2*)arg;
    while (1) {
        if (ulTaskNotifyTake(pdFALSE, portMAX_DELAY) > 0) {
            newline_parse(net->topic_cb);
        }
    }
}

static void network_event_handler(void* arg) {
    uint32_t value  = 0;
    edgelab::NetworkWE2* net = (edgelab::NetworkWE2*)arg;
    while (1) {
        if (xTaskNotifyWait(ULONG_MAX, ULONG_MAX, &value, portMAX_DELAY) == pdPASS) {
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
    _at      = &at;
    at.state = AT_STATE_LOST;
    if (at_rbuf == NULL) {
        at_rbuf = new lwRingBuffer(AT_RX_MAX_LEN);
        if (at_rbuf == NULL) {
            EL_LOGD("at_rbuf init error\n");
            return;
        }
        at.rbuf = at_rbuf;
    }

    if (at.port == NULL) {
        hx_drv_scu_set_PB9_pinmux(SCU_PB9_PINMUX_UART2_RX);
        hx_drv_scu_set_PB10_pinmux(SCU_PB10_PINMUX_UART2_TX);
        hx_drv_uart_init(USE_DW_UART_2, HX_UART2_BASE);
        at.port = hx_drv_uart_get_dev(USE_DW_UART_2);
        if (at.port == NULL) {
            EL_LOGD("uart init error\n");
            return;
        }
        at.port->uart_open(UART_BAUDRATE_115200);
        memset((void*)at.tbuf, 0, sizeof(at.tbuf));
        at.port->uart_read_udma(dma_rx, 1, (void*)dma_rx_cb);
    }

    // Parse data and trigger events
    if (xTaskCreate(at_recv_parser, "at_recv_parser", 1024, this, 1, &at_rx_parser) != pdPASS) {
        EL_LOGD("at_recv_parser create error\n");
        return;
    }
    // Handle network status change events
    if (xTaskCreate(network_event_handler, "network_event_handler", 64, this, 1, &status_handler) !=
        pdPASS) {
        EL_LOGD("network_event_handler create error\n");
        return;
    }
    at.state = AT_STATE_IDLE;

    sprintf(at.tbuf, AT_STR_HEADER AT_STR_RST AT_STR_CRLF);
    uint32_t t = 0;
    at.state   = AT_STATE_PROCESS;
    at.port->uart_write(at.tbuf, strlen(at.tbuf));
    EL_LOGD(" %s\n", at.tbuf);
    while (at.state != AT_STATE_READY) {
        if (t >= AT_LONG_TIME_MS) {
            EL_LOGD("AT RST TIMEOUT\n");
            return;
        }
        el_sleep(10);
        t += 10;
    }

    sprintf(at.tbuf, AT_STR_ECHO AT_STR_CRLF);
    err = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT ECHO ERROR : %d\n", err);
        return;
    }
    sprintf(at.tbuf, AT_STR_HEADER AT_STR_CWMODE "=1" AT_STR_CRLF);
    err = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT CWMODE ERROR : %d\n", err);
        return;
    }
    
    if (cb) this->status_cb = cb;
    this->set_status(NETWORK_IDLE);
}

void NetworkWE2::deinit() {
    at.port->uart_close();
    at.state = AT_STATE_LOST;
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
    sprintf(at.tbuf, AT_STR_HEADER AT_STR_CWJAP "=\"%s\",\"%s\"" AT_STR_CRLF, ssid, pwd);
    err = at_send(&at, AT_LONG_TIME_MS * 4);
    if (err != EL_OK) {
        EL_LOGD("AT CWJAP ERROR : %d\n", err);
        return err;
    }
    return EL_OK;
}

el_err_code_t NetworkWE2::quit() {
    el_err_code_t err = EL_OK;
    if (this->network_status == NETWORK_IDLE || this->network_status == NETWORK_LOST) {
        return EL_OK;
    }
    sprintf(at.tbuf, AT_STR_HEADER AT_STR_CWQAP AT_STR_CRLF);
    err = at_send(&at, AT_LONG_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT CWJAP ERROR : %d\n", err);
        return err;
    }
    return EL_OK;
}

el_err_code_t NetworkWE2::connect(const char* server, const char* user, const char* pass, topic_cb_t cb) {
    el_err_code_t err = EL_OK;
    if (this->network_status == NETWORK_CONNECTED) {
        return EL_OK;
    } else if (this->network_status != NETWORK_JOINED) {
        return EL_EPERM;
    }

    if (cb == NULL) {
        return EL_EINVAL;
    }
    at.cb = cb;
    this->topic_cb = cb;

    sprintf(at.tbuf,
            AT_STR_HEADER AT_STR_MQTTUSERCFG "=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"" AT_STR_CRLF,
            MQTT_CLIENT_ID,
            user,
            pass);
    err = at_send(&at, AT_SHORT_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MQTTUSERCFG ERROR : %d\n", err);
        return err;
    }

    // el_sleep(AT_SHORT_TIME_MS);
    sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTCONN "=0,\"%s\",1883,1" AT_STR_CRLF, server);
    err = at_send(&at, AT_LONG_TIME_MS);
    if (err != EL_OK) {
        EL_LOGD("AT MQTTCONN ERROR : %d\n", err);
        return err;
    }

    return EL_OK;
}

el_err_code_t NetworkWE2::disconnect() {
    el_err_code_t err = EL_OK;
    if (this->network_status != NETWORK_CONNECTED) {
        return EL_EPERM;
    }

    sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTCLEAN "=0" AT_STR_CRLF);
    err = at_send(&at, AT_SHORT_TIME_MS);
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

    sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTSUB "=0,\"%s\",%d" AT_STR_CRLF, topic, qos);
    err = at_send(&at, AT_SHORT_TIME_MS);
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

    sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTUNSUB "=0,\"%s\"" AT_STR_CRLF, topic);
    err = at_send(&at, AT_SHORT_TIME_MS);
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

    if (len < 256 - 22 - strlen(topic)) {
        sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTPUB "=0,\"%s\",\"%s\",%d,0" AT_STR_CRLF, topic, dat, qos);
        err = at_send(&at, AT_LONG_TIME_MS);
        if (err != EL_OK) {
            EL_LOGD("AT MQTTPUB ERROR : %d\n", err);
            return err;
        }
    } else if (len < sizeof(at.tbuf) - 27 - strlen(topic)) {
        sprintf(at.tbuf, AT_STR_HEADER AT_STR_MQTTPUB "RAW=0,\"%s\",%d,%d,0" AT_STR_CRLF, topic, len, qos);
        err = at_send(&at, AT_LONG_TIME_MS);
        if (err != EL_OK) {
            EL_LOGD("AT MQTTPUB ERROR : %d\n", err);
            return err;
        }
        snprintf(at.tbuf, len, "%s", dat);
        at.port->uart_write(at.tbuf, strlen(at.tbuf));
    } else {
        EL_LOGD("AT MQTTPUB ERROR : DATA TOO LONG\n");
        return EL_ENOMEM;
    }

    return EL_OK;
}

}  // namespace edgelab
