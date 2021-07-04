/**
 * @file NAR_I2C.h
 * @brief I2C API for esp32
 * @author Narukara
 * @date 2021.2
 */
#ifndef NARUKARA_I2C
#define NARUKARA_I2C

#include "esp_err.h"

void i2c_init();

esp_err_t i2c_write(uint8_t slave_addr,
                    uint8_t reg_addr,
                    size_t size,
                    uint8_t* data);

esp_err_t i2c_read(uint8_t slave_addr,
                   uint8_t reg_addr,
                   size_t size,
                   uint8_t* data);

esp_err_t i2c_write_check(uint8_t slave_addr,
                          uint8_t reg_addr,
                          size_t size,
                          uint8_t* data);

esp_err_t i2c_write_2(uint8_t slave_addr,
                    uint8_t reg_addr,
                    size_t size,
                    uint8_t* data);

esp_err_t i2c_read_2(uint8_t slave_addr,
                   uint8_t reg_addr,
                   size_t size,
                   uint8_t* data);

esp_err_t i2c_write_check_2(uint8_t slave_addr,
                          uint8_t reg_addr,
                          size_t size,
                          uint8_t* data);

#endif