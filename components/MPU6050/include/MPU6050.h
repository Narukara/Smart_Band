/**
 * @file MPU6050.h
 * @brief MPU6050 API for esp32, based on motion_driver_test.c and official
 * driver
 * @author Narukara
 * @date 2021.2
 */
#ifndef NARUKARA_MPU6050
#define NARUKARA_MPU6050

#include "esp_err.h"

void MPU6050_init();

unsigned long MPU6050_get_step();

void MPU6050_set_step(unsigned long count);

#endif