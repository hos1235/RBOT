/**
 * @file line_sensor.c
 */
#include "line_sensor.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;

static volatile uint16_t s_adc_dma[LINE_SENSOR_COUNT];
static uint16_t s_white[LINE_SENSOR_COUNT];
static uint16_t s_black[LINE_SENSOR_COUNT];
static float s_last_error;

static const int8_t s_weights[LINE_SENSOR_COUNT] = {
  LINE_WEIGHT_0, LINE_WEIGHT_1, LINE_WEIGHT_2, LINE_WEIGHT_3,
  LINE_WEIGHT_4, LINE_WEIGHT_5, LINE_WEIGHT_6, LINE_WEIGHT_7
};

static float LineSensor_Clamp01(float v)
{
  if (v < 0.0f) {
    return 0.0f;
  }
  if (v > 1.0f) {
    return 1.0f;
  }
  return v;
}

void LineSensor_Init(void)
{
  uint8_t i;

  for (i = 0; i < LINE_SENSOR_COUNT; i++) {
    s_white[i] = 3500U;
    s_black[i] = 500U;
  }
  s_last_error = 0.0f;

  HAL_ADCEx_Calibration_Start(&hadc1);
  SET_BIT(hadc1.Instance->CR2, ADC_CR2_CONT);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)s_adc_dma, LINE_SENSOR_COUNT);
}

void LineSensor_CalibrateWhite(void)
{
  uint8_t i;

  for (i = 0; i < LINE_SENSOR_COUNT; i++) {
    s_white[i] = s_adc_dma[i];
    if (s_black[i] >= s_white[i]) {
      s_black[i] = (s_white[i] > 100U) ? (s_white[i] - 100U) : 0U;
    }
  }
}

void LineSensor_CalibrateBlack(void)
{
  uint8_t i;

  for (i = 0; i < LINE_SENSOR_COUNT; i++) {
    s_black[i] = s_adc_dma[i];
    if (s_white[i] <= s_black[i]) {
      s_white[i] = s_black[i] + 100U;
    }
  }
}

const uint16_t *LineSensor_GetRaw(void)
{
  return (const uint16_t *)s_adc_dma;
}

void LineSensor_GetNormalized(float out[LINE_SENSOR_COUNT])
{
  uint8_t i;

  for (i = 0; i < LINE_SENSOR_COUNT; i++) {
    uint16_t raw = s_adc_dma[i];
    uint16_t w = s_white[i];
    uint16_t b = s_black[i];
    float denom = (float)((int32_t)w - (int32_t)b);

    if (denom < 1.0f) {
      out[i] = 0.0f;
    } else {
      out[i] = LineSensor_Clamp01(((float)raw - (float)b) / denom);
    }
  }
}

void LineSensor_GetBinary(uint8_t out[LINE_SENSOR_COUNT])
{
  float norm[LINE_SENSOR_COUNT];
  uint8_t i;

  LineSensor_GetNormalized(norm);
  for (i = 0; i < LINE_SENSOR_COUNT; i++) {
    out[i] = (norm[i] < LINE_BINARY_THRESHOLD) ? 1U : 0U;
  }
}

float LineSensor_GetError(void)
{
  uint8_t bin[LINE_SENSOR_COUNT];
  int32_t sum_w = 0;
  int32_t sum_on = 0;
  uint8_t i;

  LineSensor_GetBinary(bin);
  for (i = 0; i < LINE_SENSOR_COUNT; i++) {
    if (bin[i]) {
      sum_w += s_weights[i];
      sum_on++;
    }
  }

  if (sum_on > 0) {
    s_last_error = (float)sum_w / (float)sum_on;
    return s_last_error;
  }

  return s_last_error;
}

uint8_t LineSensor_IsAllOnLine(void)
{
  uint8_t bin[LINE_SENSOR_COUNT];
  uint8_t i;
  uint8_t count = 0U;

  LineSensor_GetBinary(bin);
  for (i = 0; i < LINE_SENSOR_COUNT; i++) {
    if (bin[i]) {
      count++;
    }
  }
  return (count >= LINE_SENSOR_COUNT) ? 1U : 0U;
}
