/**
 * @file color_id.h
 * @brief Đỏ / xanh / vàng — thay cho số 1/2/3
 */
#ifndef COLOR_ID_H
#define COLOR_ID_H

#include <stdint.h>
#include "app_config.h"

uint8_t Color_IdFromLetter(char letter);
uint8_t Color_IdFromName(const char *line);
const char *Color_Name(uint8_t color_id);
uint8_t Color_TrayFromColor(uint8_t color_id);

#endif /* COLOR_ID_H */
