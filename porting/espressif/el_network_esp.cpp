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

#include "el_network_esp.h"

#include "core/el_debug.h"

#define ESP_MAXIMUM_RETRY  10

#define ESP_WIFI_CONNECTED (1 << 0)
#define ESP_WIFI_FAILED    (1 << 1)

static int32_t retry_cnt;
static SemaphoreHandle_t el_sem_got_ip;

static bool mqtt_connected = false;

static void handler_wifi_disconnect(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    // printf("ESP_WIFI_DISCONNECTED\r\n");
    retry_cnt++;
    if (retry_cnt > ESP_MAXIMUM_RETRY) {
        if (el_sem_got_ip) {
            xSemaphoreGive(el_sem_got_ip);
        }
        return;
    }
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
}
static void handler_wifi_connect(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    // printf("ESP_WIFI_CONNECTED\r\n");
}
static void handler_sta_got_ip(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    // printf("ESP_STA_GOT_IP\r\n");
    retry_cnt = 0;
    // ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
    if (el_sem_got_ip) {
        xSemaphoreGive(el_sem_got_ip);
    }
}

static void handler_mqtt_data(void* arg, esp_event_base_t base, int32_t id, void* data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)data;
    esp_mqtt_client_handle_t client = event->client;
    topic_cb_t cb = (topic_cb_t)arg;
    // printf("MQTT_EVENT_DATA\r\n");
    if (cb) cb(event->topic, event->topic_len, event->data, event->data_len);
}

static void handler_mqtt_connect(void* arg, esp_event_base_t base, int32_t id, void* data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)data;
    esp_mqtt_client_handle_t client = event->client;
    el_net_sta_t *ns = (el_net_sta_t *)arg;
    *ns = NETWORK_CONNECTED;
    mqtt_connected = true;
    // printf("MQTT_EVENT_CONNECTED\r\n");
}

static void handler_mqtt_disconnect(void* arg, esp_event_base_t base, int32_t id, void* data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)data;
    esp_mqtt_client_handle_t client = event->client;
    el_net_sta_t *ns = (el_net_sta_t *)arg;
    *ns = NETWORK_JOINED;
    mqtt_connected = false;
    // printf("MQTT_EVENT_DISCONNECTED\r\n");
}

namespace edgelab {

void NetworkEsp::init() {
    /* Initialize NVS for config */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    esp_netif_init();
    esp_event_loop_create_default();

    /* Create a netif for WIFI station */
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    esp_netif_config.if_desc = "edgelab";
    esp_netif_config.route_prio = 128;
    esp_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    esp_wifi_set_default_wifi_sta_handlers();

    /* Initialize Wi-Fi and start STA mode */
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&init_cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    network_status = NETWORK_IDLE;
    return;
}
void NetworkEsp::deinit() {
    network_status = NETWORK_LOST;
    return;
}

el_err_code_t NetworkEsp::join(const char* ssid, const char *pwd) {
    // printf("ssid=%s pwd=%s\r\n", ssid, pwd);
    /* Set ssid and password for connection */
    wifi_config_t cfg = { 
        .sta = {
            .ssid = {0},
            .password = {0},
            .scan_method = WIFI_FAST_SCAN,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold = {.rssi = -127, .authmode = WIFI_AUTH_WPA2_PSK}
        }
    };
    if (ssid != NULL && strlen(ssid) < 32) {
        memcpy(cfg.sta.ssid, ssid, strlen(ssid) + 1);
    }
    if (pwd != NULL && strlen(pwd) < 64) {
        memcpy(cfg.sta.password, pwd, strlen(pwd) + 1);
    }
    if (esp_wifi_set_config(WIFI_IF_STA, &cfg) != ESP_OK) {
        // printf("set config failed!\r\n");
        return EL_FAILED;
    }

    /* Create semaphore and register event handler */
    el_sem_got_ip = xSemaphoreCreateBinary();
    if (el_sem_got_ip == NULL) {
        return EL_FAILED;
    }
    retry_cnt = 0;
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &handler_wifi_disconnect, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &handler_wifi_connect, esp_netif);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handler_sta_got_ip, NULL);

    if (esp_wifi_connect() != ESP_OK) {
        // printf("connect failed!\r\n");
        return EL_FAILED;
    }

    EL_LOGI("WAITING FOR IP...\r\n");
    xSemaphoreTake(el_sem_got_ip, portMAX_DELAY);
    if (retry_cnt > ESP_MAXIMUM_RETRY) {
        // printf("connect failed!\r\n");
        return EL_ETIMOUT;
    }

    network_status = NETWORK_JOINED;
    return EL_OK;
}

el_err_code_t NetworkEsp::quit() {
    esp_wifi_disconnect();
    network_status = NETWORK_IDLE;
    return EL_OK;
}


el_net_sta_t NetworkEsp::status() {
    return network_status;
}

/**
 * Connects to a server using MQTT protocol and starts the MQTT client.
 *
 * @param server The server address to connect to.
 * @param user The username for authentication (optional).
 * @param pass The password for authentication (optional).
 * @param cb The callback function to handle MQTT data events.
 *
 * @return el_err_code_t The error code indicating the result of the connection attempt.
 */
el_err_code_t NetworkEsp::connect(const char* server, const char *user, const char *pass, topic_cb_t cb) {
    char url[128] = {0};
    if (server == NULL) {
        return EL_EINVAL;
    }
    if (user != NULL && pass != NULL) {
        sprintf(url, "mqtt://%s:%s@%s", user, pass, server);
    } else {
        sprintf(url, "mqtt://%s", server);
    }

    esp_mqtt_client_config_t mqtt_cfg = {.broker = {.address = {.uri = url}}};
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_DATA, handler_mqtt_data, (void *)cb);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_CONNECTED, handler_mqtt_connect, (void *)&network_status);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_DISCONNECTED, handler_mqtt_disconnect, (void *)&network_status);
    esp_mqtt_client_start(mqtt_client);
    return EL_OK;
}

el_err_code_t NetworkEsp::subscribe(const char* topic, mqtt_qos_t qos) {
    if (!mqtt_connected) {
        return EL_EPERM;
    }
    int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, qos);
    if (msg_id < 0) {
        return EL_FAILED;
    } else if (qos != MQTT_QOS_0 && msg_id == 0) {
        return EL_EIO;
    }
    return EL_OK;
}

el_err_code_t NetworkEsp::unsubscribe(const char* topic) {
    if (!mqtt_connected) {
        return EL_EPERM;
    }
    int msg_id = esp_mqtt_client_unsubscribe(mqtt_client, topic);
    if (msg_id < 0) {
        return EL_FAILED;
    }
    return EL_OK;
}

el_err_code_t NetworkEsp::publish(const char* topic, const char* dat, uint32_t len, mqtt_qos_t qos) {
    if (!mqtt_connected) {
        return EL_EPERM;
    }

    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, dat, len, qos, 0);

    if (msg_id < 0) {
        return EL_FAILED;
    } else if (qos != MQTT_QOS_0 && msg_id == 0) {
        return EL_EIO;
    }
    return EL_OK;
}

} // namespace edgelab