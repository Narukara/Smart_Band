/**
 * @file NAR_GPIO.c
 * @brief GPIO API, mainly consists of LED, buzzer, IO0 button and MAX30102
 * interrupt
 * @author Narukara
 * @date 2021.2
 */
#include "driver/gpio.h"
#include "esp_log.h"

#define IO0 (0)
#define LED (2)
#define MAX30102_INTR (21)
#define BUZ (22)

static const char* TAG = "NAR_GPIO";

static uint8_t IO0_flag = 0;
static uint8_t IO0_double_flag = 0;

static enum { ready, running, error } status = ready;

#define fuse()                                 \
    ({                                         \
        status = error;                        \
        ESP_LOGE(TAG, "fuse at %d", __LINE__); \
    })

/**
 * @brief interrupt service function for IO0 button
 */
static void IO0_isr(void* arg) {
    if (IO0_flag) {
        IO0_flag = 0;
        IO0_double_flag = 1;
    } else {
        IO0_flag = 1;
    }
}

uint8_t NAR_GPIO_get_IO0_flag() {
    return IO0_flag;
}

void NAR_GPIO_clear_IO0_flag() {
    IO0_flag = 0;
}

uint8_t NAR_GPIO_get_IO0_double_flag() {
    return IO0_double_flag;
}

void NAR_GPIO_clear_IO0_double_flag() {
    IO0_double_flag = 0;
}

/**
 * @param[in] on_off
 * | 1 - on
 * | 0 - off
 */
void NAR_GPIO_set_BUZ(uint8_t on_off) {
    if (on_off) {
        gpio_set_level(BUZ, 0);
    } else {
        gpio_set_level(BUZ, 1);
    }
}

/**
 * @param[in] on_off
 * | 1 - on
 * | 0 - off
 */
void NAR_GPIO_set_LED(uint8_t on_off) {
    if (on_off) {
        gpio_set_level(LED, 1);
    } else {
        gpio_set_level(LED, 0);
    }
}

/**
 * @brief active low
 */
int NAR_GPIO_get_MAX30102_intr() {
    return gpio_get_level(MAX30102_INTR);
}

void NAR_GPIO_init() {
    if (status != ready) {
        return;
    }
    gpio_set_direction(BUZ, GPIO_MODE_OUTPUT);
    gpio_set_level(BUZ, 1);
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    gpio_set_level(LED, 0);
    gpio_set_direction(MAX30102_INTR, GPIO_MODE_INPUT);
    gpio_config_t config = {
        .intr_type = GPIO_PIN_INTR_POSEDGE,  // positive edge
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pin_bit_mask = (1ULL << IO0),
    };
    gpio_config(&config);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(IO0, IO0_isr, NULL);
    status = running;
    ESP_LOGI(TAG, "GPIO init");
}