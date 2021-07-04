#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "MAX30102.h"
#include "MPU6050.h"
#include "NAR_GPIO.h"
#include "NAR_I2C.h"
#include "NAR_MQTT.h"
#include "SSD1306.h"

#define STACK_SIZE 4096

static const char* TAG = "BAND";
static const char* ch_temp = "/band/temp";
static const char* ch_hr = "/band/hr";
static const char* ch_step = "/band/step";
static const char* ch_pub = "/band/pub";
static const char* ch_sub = "/band/sub";

void thread_1(void* pvParameters) {
    while (1) {
        vTaskDelay(3000 / portTICK_PERIOD_MS);
        if (NAR_GPIO_get_IO0_double_flag()) {
            if (NAR_MQTT_get_connected()) {
                NAR_MQTT_end();
            } else {
                NAR_MQTT_start();
            }
            NAR_GPIO_clear_IO0_double_flag();
        }
    }
}

static inline void heart_rate_task() {
    NAR_GPIO_clear_IO0_flag();
    SSD1306_display_hr(0, 0, NAR_MQTT_get_connected());
    uint8_t hr = MAX30102_get_hr();
    if (hr) {
        SSD1306_display_hr(1, hr, NAR_MQTT_get_connected());
        if (NAR_MQTT_get_connected()) {
            char msg[10];
            sprintf(msg, "%u", hr);
            NAR_MQTT_pub(ch_hr, msg);
        }
    } else {
        SSD1306_display_hr(2, 0, NAR_MQTT_get_connected());
    }
    vTaskDelay(3000 / portTICK_PERIOD_MS);
}

static inline void band_init() {
    NAR_GPIO_init();
    i2c_init();
    SSD1306_init();
    MAX30102_init();
    MPU6050_init();
    NAR_MQTT_init();
    TaskHandle_t xHandle = NULL;
    xTaskCreate(thread_1, "thread_1", STACK_SIZE, NULL, tskIDLE_PRIORITY,
                &xHandle);
    configASSERT(xHandle);
}

void app_main(void) {
    ESP_LOGI(TAG, "band start");
    band_init();

    // sleep mode loop
    while (1) {
        if (MAX30102_on()) {
            ESP_LOGI(TAG, "active");
            NAR_GPIO_set_LED(1);
            SSD1306_set_display(1);
            NAR_GPIO_clear_IO0_flag();
            break;
        }
    sleep:
        vTaskDelay(4500 / portTICK_PERIOD_MS);
    }

    // active mode loop
    uint8_t long_sit_count = 0;
    unsigned long last_step = 0;
    while (1) {
        double temp = MAX30102_get_temp();
        unsigned long step = MPU6050_get_step();
        if (step != 999999 && step == last_step) {
            long_sit_count++;
        } else {
            long_sit_count = 0;
            last_step = step;
        }
        if (temp > 37.3 || long_sit_count >= 30) {
            NAR_GPIO_set_BUZ(1);
        } else {
            NAR_GPIO_set_BUZ(0);
        }
        SSD1306_display_main_menu(step, temp, NAR_MQTT_get_connected());
        if (NAR_MQTT_get_connected()) {
            char msg[10];
            sprintf(msg, "%.2lf", temp);
            NAR_MQTT_pub(ch_temp, msg);
            sprintf(msg, "%lu", step);
            NAR_MQTT_pub(ch_step, msg);
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        if (NAR_GPIO_get_IO0_flag()) {
            heart_rate_task();
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        if (!MAX30102_on()) {
            ESP_LOGI(TAG, "sleep");
            NAR_GPIO_set_LED(0);
            NAR_GPIO_set_BUZ(0);
            SSD1306_set_display(0);
            // disconnect
            goto sleep;
        }
        if (NAR_GPIO_get_IO0_flag()) {
            heart_rate_task();
        }
    }
}
