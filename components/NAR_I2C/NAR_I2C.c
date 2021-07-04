/**
 * @file NAR_I2C.c
 * @brief I2C API for esp32
 * @author Narukara
 * @date 2021.2
 */
#include "driver/i2c.h"
#include "esp_log.h"
#include "string.h"

#define ACK_CHECK_EN (0x1)
#define ACK_VAL (0x0)
#define NACK_VAL (0x1)

#define I2C_FREQ (100000)
#define SCL (19)
#define SDA (18)
#define SCL_2 (17)
#define SDA_2 (16)
#define I2C_PORT I2C_NUM_0
#define I2C_PORT_2 I2C_NUM_1
#define MAX_WAIT_MS (100)

static const char* TAG = "NAR_I2C";

static enum { ready, running, error } status = ready;

#define fuse()                                 \
    ({                                         \
        status = error;                        \
        ESP_LOGE(TAG, "fuse at %d", __LINE__); \
    })

void i2c_init() {
    if (status != ready) {
        return;
    }
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = SCL,
        .sda_io_num = SDA,
        .master.clk_speed = I2C_FREQ,
    };
    if (i2c_param_config(I2C_PORT, &config) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_driver_install(I2C_PORT, config.mode, 0, 0, 0) != ESP_OK) {
        fuse();
        return;
    }

    i2c_config_t config_2 = {
        .mode = I2C_MODE_MASTER,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = SCL_2,
        .sda_io_num = SDA_2,
        .master.clk_speed = I2C_FREQ,
    };
    if (i2c_param_config(I2C_PORT_2, &config_2) != ESP_OK) {
        fuse();
        return;
    }
    if (i2c_driver_install(I2C_PORT_2, config_2.mode, 0, 0, 0) != ESP_OK) {
        fuse();
        return;
    }
    status = running;
    ESP_LOGI(TAG, "i2c init");
}

/**
 * @param[in] size size of data, 0 is OK
 * @param[in] data Nullable
 * @return ESP_OK if successful
 */
esp_err_t i2c_write(uint8_t slave_addr,
                    uint8_t reg_addr,
                    size_t size,
                    uint8_t* data) {
    if ((status != running) || (size > 0 && data == NULL)) {
        return ESP_FAIL;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE,
                          ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    if (size > 0) {
        i2c_master_write(cmd, data, size, ACK_CHECK_EN);
    }
    i2c_master_stop(cmd);
    esp_err_t ret =
        i2c_master_cmd_begin(I2C_PORT, cmd, MAX_WAIT_MS / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @param[in] size size of data, size > 0
 * @param[out] data Nullable, if data == NULL, data read from I2C will be
 * discarded
 * @return ESP_OK if successful
 */
esp_err_t i2c_read(uint8_t slave_addr,
                   uint8_t reg_addr,
                   size_t size,
                   uint8_t* data) {
    if ((status != running) || (size == 0)) {
        return ESP_FAIL;
    }
    uint8_t malloc_flag = 0;
    if (data == NULL) {
        data = malloc(sizeof(uint8_t) * size);
        if (!data) {
            return ESP_FAIL;
        }
        malloc_flag = 1;
    }
    esp_err_t ret = i2c_write(slave_addr, reg_addr, 0, NULL);
    if (ret != ESP_OK) {
        if (malloc_flag) {
            free(data);
        }
        return ret;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, slave_addr << 1 | I2C_MASTER_READ, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT, cmd, MAX_WAIT_MS / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (malloc_flag) {
        free(data);
    }
    return ret;
}

/**
 * @brief write and check, DON'T apply to volatile registers
 * @param[in] size size of data, size > 0
 * @param[in] data [in] NonNull
 */
esp_err_t i2c_write_check(uint8_t slave_addr,
                          uint8_t reg_addr,
                          size_t size,
                          uint8_t* data) {
    if ((status != running) || (size == 0) || (data == NULL)) {
        return ESP_FAIL;
    }
    esp_err_t ret = i2c_write(slave_addr, reg_addr, size, data);
    if (ret != ESP_OK) {
        return ret;
    }
    uint8_t* buf = malloc(sizeof(uint8_t) * size);
    if (!buf) {
        return ESP_FAIL;
    }
    ret = i2c_read(slave_addr, reg_addr, size, buf);
    if (ret != ESP_OK) {
        free(buf);
        return ret;
    }
    if (memcmp(data, buf, sizeof(uint8_t) * size)) {
        free(buf);
        return ESP_FAIL;
    }
    free(buf);
    return ESP_OK;
}

esp_err_t i2c_write_2(uint8_t slave_addr,
                    uint8_t reg_addr,
                    size_t size,
                    uint8_t* data) {
    if ((status != running) || (size > 0 && data == NULL)) {
        return ESP_FAIL;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE,
                          ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    if (size > 0) {
        i2c_master_write(cmd, data, size, ACK_CHECK_EN);
    }
    i2c_master_stop(cmd);
    esp_err_t ret =
        i2c_master_cmd_begin(I2C_PORT_2, cmd, MAX_WAIT_MS / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t i2c_read_2(uint8_t slave_addr,
                   uint8_t reg_addr,
                   size_t size,
                   uint8_t* data) {
    if ((status != running) || (size == 0)) {
        return ESP_FAIL;
    }
    uint8_t malloc_flag = 0;
    if (data == NULL) {
        data = malloc(sizeof(uint8_t) * size);
        if (!data) {
            return ESP_FAIL;
        }
        malloc_flag = 1;
    }
    esp_err_t ret = i2c_write_2(slave_addr, reg_addr, 0, NULL);
    if (ret != ESP_OK) {
        if (malloc_flag) {
            free(data);
        }
        return ret;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, slave_addr << 1 | I2C_MASTER_READ, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_PORT_2, cmd, MAX_WAIT_MS / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (malloc_flag) {
        free(data);
    }
    return ret;
}

esp_err_t i2c_write_check_2(uint8_t slave_addr,
                          uint8_t reg_addr,
                          size_t size,
                          uint8_t* data) {
    if ((status != running) || (size == 0) || (data == NULL)) {
        return ESP_FAIL;
    }
    esp_err_t ret = i2c_write_2(slave_addr, reg_addr, size, data);
    if (ret != ESP_OK) {
        return ret;
    }
    uint8_t* buf = malloc(sizeof(uint8_t) * size);
    if (!buf) {
        return ESP_FAIL;
    }
    ret = i2c_read_2(slave_addr, reg_addr, size, buf);
    if (ret != ESP_OK) {
        free(buf);
        return ret;
    }
    if (memcmp(data, buf, sizeof(uint8_t) * size)) {
        free(buf);
        return ESP_FAIL;
    }
    free(buf);
    return ESP_OK;
}