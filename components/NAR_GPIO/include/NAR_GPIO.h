/**
 * @file NAR_GPIO.h
 * @brief GPIO API, mainly consists of LED, buzzer, IO0 button and MAX30102
 * interrupt
 * @author Narukara
 * @date 2021.2
 */
#ifndef NARUKARA_GPIO
#define NARUKARA_GPIO

#include "esp_types.h"

void NAR_GPIO_init();

uint8_t NAR_GPIO_get_IO0_flag();

void NAR_GPIO_clear_IO0_flag();

uint8_t NAR_GPIO_get_IO0_double_flag();

void NAR_GPIO_clear_IO0_double_flag();

void NAR_GPIO_set_BUZ(uint8_t on_off);

void NAR_GPIO_set_LED(uint8_t on_off);

int NAR_GPIO_get_MAX30102_intr();

#endif