/**
 * @file MAX30120.h
 * @brief MAX30102 API, referenced MAXREFDES117# project from Maxim Integrated
 * Products, Inc.
 * @author Narukara
 * @date 2021.2
 */

#ifndef NARUKARA_MAX30102
#define NARUKARA_MAX30102

#include "esp_types.h"

void MAX30102_init();

uint8_t MAX30102_get_hr();

double MAX30102_get_temp();

uint8_t MAX30102_on();

#endif