/**
 * @file SSD1306.c
 * @brief SSD1306 API for esp32, referenced uNetworking/SSD1306 project.
 * the url is https://github.com/uNetworking/SSD1306
 * @author Narukara
 * @date 2021.2
 */
#include "NAR_I2C.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "font.h"
#include "string.h"

#define SSD1306 (0x3c)
#define COMD (0x80)
#define DATA (0x40)

static const char* TAG = "SSD1306";

/**
 *      buffer(index) <-> display
 * |   0|   1|   2|........| 126| 127|
 * | 128|....                   | 255|
 * | 256|....                   |    |
 * | 384|....                   |    |
 * | 512|....                   |    |
 * | 640|....                   |    |
 * | 768|....                   |    |
 * | 896| 897| 898|........     |1023|
 *
 * every || stands for one byte
 *
 *         |0|
 *         |1|
 *         |0|
 *  || --> |1|
 *         |0|
 *         |1|
 *         |0|
 *         |1|
 *
 * Therefore, there are 8 row, 128 col in totol
 *
 *                                  <-8 bits->
 * a 16x8 character -->    /\    | | | | | | | | |
 *                       16 bits | | | | | | | | |
 *                         \/
 * it has 2 row and 8 col
 *
 *                                              <-col->
 * a (8*row) x col character -->    /\    | | | | | | | | | |
 *                                (8*row) | | | | | | | | | |
 *                                  \/    | | | | | | | | | |
 *                                        | | | | | | | | | |
 */
static uint8_t buffer[1024];

static enum { ready, running, error } status = ready;

#define fuse()                                 \
    ({                                         \
        status = error;                        \
        ESP_LOGE(TAG, "fuse at %d", __LINE__); \
    })

void SSD1306_init() {
    if (status != ready) {
        return;
    }
    uint8_t init[] = {0x8D, COMD, 0x14, COMD, 0xAE, COMD, 0x20,
                      COMD, 0x00, COMD, 0x21, COMD, 0x00, COMD,
                      0x7F, COMD, 0x22, COMD, 0x00, COMD, 0x07};
    esp_err_t ret = i2c_write(SSD1306, COMD, 21, init);
    if (ret != ESP_OK) {
        fuse();
        return;
    }
    status = running;
    ESP_LOGI(TAG, "ssd1306 init");
}

/**
 * @brief push buffer to GDDRAM to display it
 */
static void SSD1306_transfer_buffer() {
    if (status != running) {
        return;
    }
    uint8_t* p = buffer;
    for (uint8_t i = 0; i < 64; i++) {
        if (i2c_write(SSD1306, DATA, 16, p) != ESP_OK) {
            fuse();
            return;
        }
        p += 16;
    }
    memset(buffer, 0, sizeof(uint8_t) * 1024);
}

/**
 * @param[in] x 0 <= x <= 127
 * @param[in] y 0 <= y <= 63
 *
 * @description
 *
 *  0------------------127>  X axis
 *  |
 *  |
 *  |
 *  |
 *  63
 *  \/  Y axis
 *
 */
static void SSD1306_set_pixel(uint8_t x, uint8_t y) {
    if (x <= 127 && y <= 63) {
        buffer[((y & 0xf8) << 4) + x] |= 1 << (y & 7);
    } else {
        ESP_LOGW(TAG, "set_pixel out of range");
    }
}

/**
 * @brief print a character (or image) in buffer
 * @param[in] row number of rows occupied by character, 1 row == 8 bits == 1
 * byte, 1 <= row <= 8
 * @param[in] col Number of columns occupied by character, 1 col == 1 bit, 1 <=
 * col <= 128
 * @param[in] font font of character
 * @param[in] offset begin position (index of buffer), 0 <= offset <= 1023
 */
static void SSD1306_set_char(uint8_t row,
                             uint8_t col,
                             const uint8_t font[],
                             uint16_t offset) {
    uint16_t index = 0;
    for (uint16_t c = 0; c < col; c++) {
        for (uint16_t r = 0; r < row; r++) {
            uint16_t temp = offset + (r << 7) + c;
            if (temp > 1023) {
                ESP_LOGW(TAG, "set_char out of range");
                return;
            }
            buffer[temp] = font[index++];
        }
    }
}

/**
 * @brief set display on/off
 * @param[in] on_off
 * | 1 - on
 * | 0 - off
 */
void SSD1306_set_display(uint8_t on_off) {
    if (status != running) {
        return;
    }
    if (on_off) {
        if (i2c_write(SSD1306, COMD, 1, (uint8_t[]){0xAF}) != ESP_OK) {
            fuse();
        }
    } else {
        if (i2c_write(SSD1306, COMD, 1, (uint8_t[]){0xAE}) != ESP_OK) {
            fuse();
        }
    }
}

static void display_header(uint8_t wifi) {
    uint64_t min = esp_timer_get_time() / 60000000;
    uint16_t hour = min / 60;
    min %= 60;
    SSD1306_set_char(2, 8, font_num_2_8[hour / 10 % 10], 0);
    SSD1306_set_char(2, 8, font_num_2_8[hour % 10], 8);
    SSD1306_set_char(2, 8, font_mao_2_8, 16);
    SSD1306_set_char(2, 8, font_num_2_8[min / 10], 24);
    SSD1306_set_char(2, 8, font_num_2_8[min % 10], 32);
    if (wifi) {
        SSD1306_set_char(2, 16, font_wifi_2_16, 80);
    }
    SSD1306_set_char(2, 31, font_battery_2_31, 96);
    for (int i = 0; i < 128; i++) {
        SSD1306_set_pixel(i, 15);
    }
}

void SSD1306_display_main_menu(unsigned long step, double temp, uint8_t wifi) {
    if (status != running) {
        return;
    }
    display_header(wifi);
    SSD1306_set_char(3, 24, font_step_3_24, 256);
    SSD1306_set_char(2, 8, font_S_2_8, 408);
    SSD1306_set_char(2, 8, font_T_2_8, 416);
    SSD1306_set_char(2, 8, font_E_2_8, 424);
    SSD1306_set_char(2, 8, font_P_2_8, 432);
    SSD1306_set_char(3, 24, font_temp_3_24, 320);
    SSD1306_set_char(2, 8, font_T_2_8, 472);
    SSD1306_set_char(2, 8, font_E_2_8, 480);
    SSD1306_set_char(2, 8, font_M_2_8, 488);
    SSD1306_set_char(2, 8, font_P_2_8, 496);
    if (step <= 99999) {
        uint8_t index = 0;
        do {
            SSD1306_set_char(3, 12, font_num_3_12[step % 10],
                             688 - 12 * index++);
            step /= 10;
        } while (step != 0);
    } else {
        SSD1306_set_char(3, 12, font_plus_3_12, 700);
        for (int i = 0; i < 5; i++) {
            SSD1306_set_char(3, 12, font_num_3_12[9], 688 - 12 * i);
        }
    }
    temp *= 10;
    uint16_t foo = (uint16_t)temp;
    if (temp - foo >= 0.5) {
        foo++;
    }
    SSD1306_set_char(3, 12, font_num_3_12[foo / 100 % 10], 720);
    SSD1306_set_char(3, 12, font_num_3_12[foo / 10 % 10], 732);
    SSD1306_set_char(3, 12, font_num_3_12[foo % 10], 756);
    SSD1306_set_char(3, 12, font_dot_3_12, 744);
    SSD1306_transfer_buffer();
}

/**
 * @param[in] type
 * | 0 - waiting
 * | 1 - success
 * | 2 - fail
 */
void SSD1306_display_hr(uint8_t type, uint8_t hr, uint8_t wifi) {
    if (status != running) {
        return;
    }
    display_header(wifi);
    SSD1306_set_char(3, 24, font_hr_3_24, 416);
    if (type == 0) {
        for (int i = 0; i < 3; i++) {
            SSD1306_set_char(3, 12, font_dot_3_12, 456 + i * 12);
        }
    } else if (type == 1) {
        SSD1306_set_char(3, 12, font_num_3_12[hr % 10], 480);
        SSD1306_set_char(3, 12, font_num_3_12[hr / 10 % 10], 468);
        if (hr > 99) {
            SSD1306_set_char(3, 12, font_num_3_12[hr / 100], 456);
        }
    } else {
        SSD1306_set_char(3, 24, font_fail_3_24, 456);
    }
    SSD1306_transfer_buffer();
}