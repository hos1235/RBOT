/**
 * @file line_sensor.h
 * @brief Cảm biến line 8 kênh analog (ADC1 + DMA)
 */
#ifndef LINE_SENSOR_H
#define LINE_SENSOR_H

#include <stdint.h>
#include "app_config.h"

void LineSensor_Init(void);
void LineSensor_CalibrateWhite(void);
void LineSensor_CalibrateBlack(void);

const uint16_t *LineSensor_GetRaw(void);
void LineSensor_GetNormalized(float out[LINE_SENSOR_COUNT]);
void LineSensor_GetBinary(uint8_t out[LINE_SENSOR_COUNT]);
float LineSensor_GetError(void);
uint8_t LineSensor_IsAllOnLine(void);

#endif /* LINE_SENSOR_H */
