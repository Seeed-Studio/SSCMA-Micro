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

extern const uint8_t ca_crt[]     asm("_binary_mqtt_ca_crt_start");

void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) 
{
    edgelab::NetworkEsp *net = (edgelab::NetworkEsp *)arg;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        printf("got ip\n");
        net->set_status(NETWORK_JOINED);
        memcpy(&net->_ip.ip.addr, &event->ip_info.ip, 4);
        memcpy(&net->_ip.netmask.addr, &event->ip_info.netmask, 4);
        memcpy(&net->_ip.gateway.addr, &event->ip_info.gw, 4);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("wifi disconnected\n");
        net->set_status(NETWORK_IDLE);
    }
}

static void mqtt_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)data;
    edgelab::NetworkEsp *net = (edgelab::NetworkEsp *)arg;
    switch ((esp_mqtt_event_id_t)id) {
        case MQTT_EVENT_CONNECTED:
            net->set_status(NETWORK_CONNECTED);
            break;
        case MQTT_EVENT_DISCONNECTED:
            net->set_status(NETWORK_JOINED);
            break;
        case MQTT_EVENT_DATA:
            if (net->topic_cb) {
                net->topic_cb(event->topic, event->topic_len, event->data, event->data_len);
            }
            break;
        default:
            break;
    }
}

namespace edgelab {

void NetworkEsp::init(status_cb_t cb) {
    if (this->network_status != NETWORK_LOST) {
        return;
    }
    if (cb) this->status_cb = cb;
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

    this->set_status(NETWORK_IDLE);
    return;
}

void NetworkEsp::deinit() {
    if (this->network_status == NETWORK_LOST) {
        return;
    }

    esp_wifi_stop();
    esp_wifi_deinit();
    esp_wifi_clear_default_wifi_driver_and_handlers(esp_netif);
    esp_netif_destroy(esp_netif);
    esp_netif = NULL;

    this->set_status(NETWORK_LOST);
    return;
}

el_err_code_t NetworkEsp::join(const char* ssid, const char *pwd) {
    if (this->network_status == NETWORK_JOINED || this->network_status == NETWORK_CONNECTED) {
        return EL_OK;
    } else if (network_status == NETWORK_LOST) {
        return EL_EPERM;
    }

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, (void *)this);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, (void *)this);

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
        printf("set config failed!\r\n");
        return EL_FAILED;
    }

    if (esp_wifi_connect() != ESP_OK) {
        printf("wifi connect failed!\r\n");
        return EL_FAILED;
    }
    return EL_OK;
}

el_err_code_t NetworkEsp::quit() {
    if (this->network_status == NETWORK_IDLE || this->network_status == NETWORK_LOST) {
        return EL_OK;
    }
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);
    esp_wifi_disconnect();
    return EL_OK;
}

el_err_code_t NetworkEsp::set_mdns(mdns_record_t record) {
    if (this->network_status == NETWORK_LOST) {
        return EL_EPERM;
    }

    mdns_init();
    mdns_hostname_set("xiao_esp32s3");
    mdns_instance_name_set("xiao_esp32s3");

    char port_str[6];
    sprintf(port_str, "%d", record.port);
    mdns_txt_item_t serviceTxtData[] = {
        {MDNS_ITEM_PROTOCAL, record.protocol},
        {MDNS_ITEM_SERVER, record.server},
        {MDNS_ITEM_PORT, port_str},
        {MDNS_ITEM_DEST, record.destination},
        {MDNS_ITEM_AUTH, record.authentication},
    };
    mdns_service_add("xiao_esp32s3", "_sscma", "_tcp", 3141, serviceTxtData, 5);

    return EL_OK;
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
el_err_code_t NetworkEsp::connect(mqtt_server_config_t mqtt_cfg, topic_cb_t cb) {
    if (this->network_status == NETWORK_CONNECTED) {
        return EL_OK;
    } else if (this->network_status != NETWORK_JOINED) {
        return EL_EPERM;
    }
    if (cb) this->topic_cb = cb;

    esp_mqtt_client_config_t esp_mqtt_cfg = {
        .broker = {
            .address = {
                .hostname = mqtt_cfg.address,
                .transport = (mqtt_cfg.use_ssl) ? MQTT_TRANSPORT_OVER_SSL : MQTT_TRANSPORT_OVER_TCP,
                .port = mqtt_cfg.port,
            }
        },
        .credentials = {
            .username = mqtt_cfg.username,
            .client_id = mqtt_cfg.client_id,
            .authentication = {
                .password = mqtt_cfg.password
            }
        },
        .task = {
            .priority = 3,
            .stack_size = 8192
        }
    };
    if (mqtt_cfg.use_ssl) {
        esp_mqtt_cfg.broker.verification.certificate = (const char *)ca_crt;
        // printf("ca_crt: %s\n", ca_crt);
    }
    mqtt_client = esp_mqtt_client_init(&esp_mqtt_cfg);

    esp_mqtt_client_register_event(mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, 
                                    mqtt_event_handler, (void *)this);
    // printf("task stack: %d, prio: %d\n", mqtt_client->config->task_stack, mqtt_client->config->task_prio);
    esp_mqtt_client_start(mqtt_client);
    return EL_OK;
}

el_err_code_t NetworkEsp::disconnect() {
    if (this->network_status != NETWORK_CONNECTED) {
        return EL_EPERM;
    }
    esp_mqtt_client_disconnect(mqtt_client);
    esp_mqtt_client_stop(mqtt_client);
    return EL_OK;
}

el_err_code_t NetworkEsp::subscribe(const char* topic, mqtt_qos_t qos) {
    if (this->network_status != NETWORK_CONNECTED) {
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
    if (this->network_status != NETWORK_CONNECTED) {
        return EL_EPERM;
    }
    int msg_id = esp_mqtt_client_unsubscribe(mqtt_client, topic);
    if (msg_id < 0) {
        return EL_FAILED;
    }
    return EL_OK;
}

el_err_code_t NetworkEsp::publish(const char* topic, const char* dat, uint32_t len, mqtt_qos_t qos) {
    if (this->network_status != NETWORK_CONNECTED) {
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