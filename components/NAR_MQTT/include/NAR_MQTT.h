/**
 * @file NAR_MQTT.c
 * @brief MQTT API for esp32
 * @author Narukara
 * @date 2021.2
 */
#ifndef NARUKARA_MQTT
#define NARUKARA_MQTT

#include "esp_types.h"

void NAR_MQTT_init();

void NAR_MQTT_start();

void NAR_MQTT_end();

uint8_t NAR_MQTT_get_connected();

void NAR_MQTT_pub(const char* topic, const char* data);

#endif