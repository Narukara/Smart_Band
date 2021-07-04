/**
 * @file SSD1306.h
 * @brief SSD1306 API for esp32, referenced uNetworking/SSD1306 project.
 * the url is https://github.com/uNetworking/SSD1306
 * @author Narukara
 * @date 2021.2
 */
#ifndef NARUKARA_SSD1306
#define NARUKARA_SSD1306

#include "esp_types.h"

void SSD1306_init();
void SSD1306_set_display(uint8_t on_off);
void SSD1306_display_main_menu(unsigned long step, double temp, uint8_t wifi);
void SSD1306_display_hr(uint8_t type, uint8_t hr, uint8_t wifi);

#endif
