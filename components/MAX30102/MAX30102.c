/**
 * @file MAX30120.c
 * @brief MAX30102 API, referenced MAXREFDES117# project from Maxim Integrated
 * Products, Inc.
 * @author Narukara
 * @date 2021.2
 */
#include "NAR_GPIO.h"
#include "NAR_I2C.h"
#include "algorithm.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"

#define MAX30102 (0x57)

#define INTR_STATUS_1 (0x00)
#define INTR_STATUS_2 (0x01)
#define INTR_ENABLE_1 (0x02)
#define INTR_ENABLE_2 (0x03)
#define FIFO_WR_PTR (0x04)
#define OVF_COUNTER (0x05)
#define FIFO_RD_PTR (0x06)
#define FIFO_DATA (0x07)
#define FIFO_CONFIG (0x08)
#define MODE_CONFIG (0x09)
#define SPO2_CONFIG (0x0A)
#define LED1_PA (0x0C)
#define LED2_PA (0x0D)
#define PILOT_PA (0x10)
#define MULTI_LED_CTRL1 (0x11)
#define MULTI_LED_CTRL2 (0x12)
#define TEMP_INTR (0x1F)
#define TEMP_FRAC (0x20)
#define TEMP_CONFIG (0x21)
#define PROX_INT_THRESH (0x30)
#define REV_ID (0xFE)
#define PART_ID (0xFF)

static const char* TAG = "MAX30102";

static enum { ready, running, error } status = ready;

#define fuse()                                 \
    ({                                         \
        status = error;                        \
        ESP_LOGE(TAG, "fuse at %d", __LINE__); \
    })

void MAX30102_init() {
    if (status != ready) {
        return;
    }

    if (i2c_write(MAX30102, MODE_CONFIG, 1, (uint8_t[]){0x40}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check(MAX30102, INTR_ENABLE_1, 1, (uint8_t[]){0xC0}) !=
        ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check(MAX30102, INTR_ENABLE_2, 1, (uint8_t[]){0x00}) !=
        ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write(MAX30102, FIFO_WR_PTR, 1, (uint8_t[]){0x00}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write(MAX30102, OVF_COUNTER, 1, (uint8_t[]){0x00}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write(MAX30102, FIFO_RD_PTR, 1, (uint8_t[]){0x00}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check(MAX30102, FIFO_CONFIG, 1, (uint8_t[]){0x1F}) !=
        ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check(MAX30102, MODE_CONFIG, 1, (uint8_t[]){0x83}) !=
        ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check(MAX30102, SPO2_CONFIG, 1, (uint8_t[]){0x27}) !=
        ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check(MAX30102, LED1_PA, 1, (uint8_t[]){0x24}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check(MAX30102, LED2_PA, 1, (uint8_t[]){0x24}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check(MAX30102, PILOT_PA, 1, (uint8_t[]){0x7F}) != ESP_OK) {
        fuse();
        return;
    }

    if (i2c_write_2(MAX30102, MODE_CONFIG, 1, (uint8_t[]){0x40}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check_2(MAX30102, INTR_ENABLE_1, 1, (uint8_t[]){0xC0}) !=
        ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check_2(MAX30102, INTR_ENABLE_2, 1, (uint8_t[]){0x00}) !=
        ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_2(MAX30102, FIFO_WR_PTR, 1, (uint8_t[]){0x00}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_2(MAX30102, OVF_COUNTER, 1, (uint8_t[]){0x00}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_2(MAX30102, FIFO_RD_PTR, 1, (uint8_t[]){0x00}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check_2(MAX30102, FIFO_CONFIG, 1, (uint8_t[]){0x1F}) !=
        ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check_2(MAX30102, MODE_CONFIG, 1, (uint8_t[]){0x83}) !=
        ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check_2(MAX30102, SPO2_CONFIG, 1, (uint8_t[]){0x27}) !=
        ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check_2(MAX30102, LED1_PA, 1, (uint8_t[]){0x24}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check_2(MAX30102, LED2_PA, 1, (uint8_t[]){0x24}) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_write_check_2(MAX30102, PILOT_PA, 1, (uint8_t[]){0x7F}) != ESP_OK) {
        fuse();
        return;
    }

    status = running;
    ESP_LOGI(TAG, "max30102 init");
}

/**
 * @param[in] on_off
 * | 0 - off
 * | 1 - on
 *
 * @param[in] which
 * | 0 - hr
 * | 1 - wear
 */
void MAX30102_shutdown(uint8_t on_off, uint8_t which) {
    if (status != running) {
        return;
    }
    if (on_off) {
        if (which == 0) {
            if (i2c_write_check(MAX30102, MODE_CONFIG, 1, (uint8_t[]){0x03}) !=
                ESP_OK) {
                fuse();
            }
        } else {
            if (i2c_write_check_2(MAX30102, MODE_CONFIG, 1,
                                  (uint8_t[]){0x03}) != ESP_OK) {
                fuse();
            }
        }
    } else {
        if (which == 0) {
            if (i2c_write_check(MAX30102, MODE_CONFIG, 1, (uint8_t[]){0x83}) !=
                ESP_OK) {
                fuse();
            }
        } else {
            if (i2c_write_check_2(MAX30102, MODE_CONFIG, 1,
                                  (uint8_t[]){0x83}) != ESP_OK) {
                fuse();
            }
        }
    }
}

/**
 * @param[out] ir_led ir led data read from fifo
 *
 * @param[in] which
 * | 0 - hr
 * | 1 - wear
 *
 * @return ESP_OK if successful
 */
static esp_err_t MAX30102_read_fifo(uint32_t* ir_led, uint8_t which) {
    if (status != running) {
        return ESP_FAIL;
    }
    uint8_t read_buffer[6];
    uint8_t foo;

    if (which == 0) {
        if (i2c_read(MAX30102, INTR_STATUS_1, 1, &foo) != ESP_OK) {
            fuse();
            return ESP_FAIL;
        }

        if (i2c_read(MAX30102, INTR_STATUS_2, 1, &foo) != ESP_OK) {
            fuse();
            return ESP_FAIL;
        }

        if (i2c_read(MAX30102, FIFO_DATA, 6, read_buffer) != ESP_OK) {
            fuse();
            return ESP_FAIL;
        }
    } else {
        if (i2c_read_2(MAX30102, INTR_STATUS_1, 1, &foo) != ESP_OK) {
            fuse();
            return ESP_FAIL;
        }

        if (i2c_read_2(MAX30102, INTR_STATUS_2, 1, &foo) != ESP_OK) {
            fuse();
            return ESP_FAIL;
        }

        if (i2c_read_2(MAX30102, FIFO_DATA, 6, read_buffer) != ESP_OK) {
            fuse();
            return ESP_FAIL;
        }
    }
    uint32_t temp;
    *ir_led = 0;
    temp = read_buffer[3];
    temp <<= 16;
    *ir_led += temp;
    temp = read_buffer[4];
    temp <<= 8;
    *ir_led += temp;
    temp = read_buffer[5];
    *ir_led += temp;
    *ir_led &= 0x03FFFF;
    return ESP_OK;
}

#define THRESHOLD 90000

/**
 * @brief detect whether there is something nearby
 * | that is to say, whether the band is wearing
 *
 * @return 1 if the band is wearing
 */
uint8_t MAX30102_on() {
    if (status != running) {
        return 0;
    }
    MAX30102_shutdown(1, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    uint32_t ir;
    if (MAX30102_read_fifo(&ir, 1) == ESP_OK) {
        MAX30102_shutdown(0, 1);
        // ESP_LOGI(TAG, "%u", ir);
        return (ir > THRESHOLD);
    }
    // already fused in read_fifo
    return 0;
}

/**
 * @brief get temperature
 */
double MAX30102_get_temp() {
    if (status != running) {
        return 0.0;
    }
    u_int8_t buf[1] = {0x01};
    double temp = 0.0;
    if (i2c_write_2(MAX30102, TEMP_CONFIG, 1, buf) != ESP_OK) {
        fuse();
        return 0.0;
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
    if (i2c_read_2(MAX30102, TEMP_INTR, 1, buf) != ESP_OK) {
        fuse();
        return 0.0;
    }
    temp += (buf[0] < 128) ? buf[0] : (((int16_t)buf[0]) - 256);
    if (i2c_read_2(MAX30102, TEMP_FRAC, 1, buf) != ESP_OK) {
        fuse();
        return 0.0;
    }
    return temp + 0.0625 * buf[0];
}

#define BUFFER_LENGTH (500)
static uint32_t ir_buffer[BUFFER_LENGTH];
#define MAX_HR (200)
#define MAX_DIFF (25)
#define MIN(x, y) (((x) > (y)) ? (y) : (x))
// #define STRICT

/**
 * @return heart rate
 * | if failed, return 0
 */
uint8_t MAX30102_get_hr() {
    if (status != running) {
        return 0;
    }
    MAX30102_shutdown(1, 0);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    int32_t hr[2];
    int8_t valid[2];
    for (uint8_t times = 0; times < 2; times++) {
        uint8_t foo;
        if (i2c_read(MAX30102, INTR_STATUS_1, 1, &foo) != ESP_OK) {
            fuse();
            return 0;
        }
        for (int i = 0; i < BUFFER_LENGTH; i++) {
            while (NAR_GPIO_get_MAX30102_intr()) {
            }
            if (MAX30102_read_fifo(ir_buffer + i, 0) != ESP_OK) {
                return 0;
            }
        }
        maxim_heart_rate_saturation(ir_buffer, BUFFER_LENGTH, &hr[times],
                                    &valid[times]);
        ESP_LOGI(TAG, "%i %i", hr[times], valid[times]);
        if (hr[times] >= MAX_HR) {
            valid[times] = 0;
        }
    }
    MAX30102_shutdown(0, 0);
#ifdef STRICT
    if (valid[0] && valid[1]) {
        int32_t temp = hr[0] - hr[1];
        if (temp < MAX_DIFF && temp > -MAX_DIFF) {
            return MIN(hr[0], hr[1]);
        }
        return 0;
    }
    return 0;
#else
    if (valid[0]) {
        if (valid[1]) {
            return MIN(hr[0], hr[1]);
        } else {
            return hr[0];
        }
    } else if (valid[1]) {
        return hr[1];
    }
    return 0;
#endif
}