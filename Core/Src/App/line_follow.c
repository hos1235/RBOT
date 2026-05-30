/**
 * @file line_follow.c
 */
#include "line_follow.h"
#include "line_sensor.h"
#include "motor.h"
#include "app_config.h"

static float s_last_error;
static float s_last_derivative;
static uint8_t s_running;

void LineFollow_Init(void)
{
  LineFollow_Reset();
}

void LineFollow_Reset(void)
{
  s_last_error = 0.0f;
  s_last_derivative = 0.0f;
  s_running = 0U;
}

void LineFollow_Stop(void)
{
  s_running = 0U;
  Motor_StopAll();
}

void LineFollow_Update(void)
{
  float error;
  float derivative;
  float steer;
  int16_t left;
  int16_t right;

  s_running = 1U;
  error = LineSensor_GetError();
  derivative = error - s_last_error;
  s_last_error = error;

  steer = LINE_FOLLOW_KP * error + LINE_FOLLOW_KD * derivative;
  left = (int16_t)(LINE_FOLLOW_BASE_SPEED - steer);
  right = (int16_t)(LINE_FOLLOW_BASE_SPEED + steer);

  Motor_SetBoth(left, right);
}
