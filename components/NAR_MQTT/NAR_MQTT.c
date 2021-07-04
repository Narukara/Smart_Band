/**
 * @file NAR_MQTT.c
 * @brief MQTT API for esp32
 * @author Narukara
 * @date 2021.2
 */
#include "esp_err.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

static const char* TAG = "NAR_MQTT";

static enum { ready, running, error } status = ready;

#define fuse()                                 \
    ({                                         \
        status = error;                        \
        ESP_LOGE(TAG, "fuse at %d", __LINE__); \
    })

static uint8_t connected = 0;
static esp_mqtt_client_handle_t client = NULL;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            connected = 1;
            esp_mqtt_client_subscribe(client, "/esp32/sub", 1);
            break;

        case MQTT_EVENT_DISCONNECTED:
            connected = 0;
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x",
                         event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x",
                         event->error_handle->esp_tls_stack_err);
            } else if (event->error_handle->error_type ==
                       MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x",
                         event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x",
                         event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void* handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void* event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base,
             event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void) {
    if (client == NULL) {
        const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtts://49.235.143.220:1883",
        .cert_pem = (const char *)"-----BEGIN CERTIFICATE-----\nMIIDyzCCArOgAwIBAgIJAMtNfJ3gcyScMA0GCSqGSIb3DQEBCwUAMHwxCzAJBgNVBAYTAkNOMRAwDgYDVQQIDAdCZWlKaW5nMRAwDgYDVQQHDAdCZWlKaW5nMREwDwYDVQQKDAhNZWRpY2luZTERMA8GA1UEAwwITmFydWthcmExIzAhBgkqhkiG9w0BCQEWFGh1cnVpMjE3QGZveG1haWwuY29tMB4XDTIwMTAyNDE1NTEzNFoXDTMwMTAyMjE1NTEzNFowfDELMAkGA1UEBhMCQ04xEDAOBgNVBAgMB0JlaUppbmcxEDAOBgNVBAcMB0JlaUppbmcxETAPBgNVBAoMCE1lZGljaW5lMREwDwYDVQQDDAhOYXJ1a2FyYTEjMCEGCSqGSIb3DQEJARYUaHVydWkyMTdAZm94bWFpbC5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDGat1oSjNXLUEdLO30B/a1cszVK6Hl2IOtVm5EHJtcbzoHFUrBcMPRbvwf8NAF7H23w4q+ZaWwhmZALZOdO2mp7/e7gIIqmXGB6bD6tD+fQtlRe2yJqXLHtx81blv4/VQUUdlHgO0o7OlQl0tylFP+CjCMHUq+gmj/pg0TcnRC39tiQf1rqio0URdhktMedGgBkC4g4ZWHYm/k06MhoHsFqa2heizRmPGfYBdHlYCwSgF2lduR7ea2xwg8GTm7NRbspt1j4orjTcbeiR7egm0WKGlD4y5U7UW3QisKSYuNBhD81aVO2IXx8G/rgQgqnuAX/F+TfR0VMuE76x+/GV49AgMBAAGjUDBOMB0GA1UdDgQWBBS/4YpWTMEb8ztB4yUQLrZKI8IgNDAfBgNVHSMEGDAWgBS/4YpWTMEb8ztB4yUQLrZKI8IgNDAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBCwUAA4IBAQA2YgY1UgIXwiA0Q38jjUaZbGPNXlYi1leSVBT1swdSpWuPp1Nfhp/V95RUDq830fi5xGZbJ/yFZgNtYv8MwtoFkpRWbIMuQQS7jSzAP0u5X9u0+xpPFzqrn0pzevemtti5YOeVH748o87yjWgfij+90NyvNEbhETHZiA/aN0ncHluY/eVWp70ZqeKCvbBlXM5zJHxjmFvjCmfoG5KyZFsdf0WyxpRsFvK9m1B8N+S5l8XMaxt7M1jkTWVQbso3Db8nzd1H8ylSZLmuTxE/u393egCL/z0s3hiLq+0YiigyTWYM1JQD79dnwDlF8iW0FUwdusKvrSPCW1vrKRtnxIPI\n-----END CERTIFICATE-----",
        .username = "esp32",
        .password = "esp32",
        };
        client = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID,
                                       mqtt_event_handler, client);
    }
    esp_mqtt_client_start(client);
}

void NAR_MQTT_init() {
    if (status != ready) {
        return;
    }
    if (nvs_flash_init() != ESP_OK) {
        fuse();
        return;
    }
    if (esp_netif_init() != ESP_OK) {
        fuse();
        return;
    }
    if (esp_event_loop_create_default() != ESP_OK) {
        fuse();
        return;
    }
    status = running;
    ESP_LOGI(TAG, "mqtt init");
}

void NAR_MQTT_start() {
    if (status != running || connected) {
        return;
    }
    if (example_connect() != ESP_OK) {
        fuse();
        return;
    }
    mqtt_app_start();
}

void NAR_MQTT_end() {
    if (status != running || !connected) {
        return;
    }
    if (esp_mqtt_client_stop(client) != ESP_OK) {
        fuse();
    }
    if (example_disconnect() != ESP_OK) {
        fuse();
    }
    connected = 0;
}

uint8_t NAR_MQTT_get_connected() {
    return connected;
}

void NAR_MQTT_pub(const char* topic, const char* data) {
    if (status != running || connected != 1) {
        return;
    }
    esp_mqtt_client_publish(client, topic, data, 0, 1, 0);
}