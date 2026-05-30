/**
 * @file motor.c
 */
#include "motor.h"
#include "main.h"
#include "app_config.h"
#include <stdlib.h>

extern TIM_HandleTypeDef htim1;

static void Motor_SetDirection(MotorId_t id, int16_t speed)
{
  GPIO_TypeDef *port;
  uint16_t pin1;
  uint16_t pin2;

  if (id == MOTOR_LEFT) {
    pin1 = AIN1_Pin;
    pin2 = AIN2_Pin;
    port = AIN1_GPIO_Port;
  } else {
    pin1 = BIN1_Pin;
    pin2 = BIN2_Pin;
    port = BIN1_GPIO_Port;
  }

  if (speed > 0) {
    HAL_GPIO_WritePin(port, pin1, GPIO_PIN_SET);
    HAL_GPIO_WritePin(port, pin2, GPIO_PIN_RESET);
  } else if (speed < 0) {
    HAL_GPIO_WritePin(port, pin1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(port, pin2, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(port, pin1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(port, pin2, GPIO_PIN_RESET);
  }
}

static void Motor_SetPwm(MotorId_t id, uint16_t duty)
{
  if (id == MOTOR_LEFT) {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
  } else {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, duty);
  }
}

void Motor_Init(void)
{
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
  __HAL_TIM_MOE_ENABLE(&htim1);

  HAL_GPIO_WritePin(STBY_GPIO_Port, STBY_Pin, GPIO_PIN_SET);
  Motor_StopAll();
}

void Motor_Set(MotorId_t id, int16_t speed_percent)
{
  int16_t speed = speed_percent;

  if (speed > MOTOR_SPEED_MAX) {
    speed = MOTOR_SPEED_MAX;
  } else if (speed < -MOTOR_SPEED_MAX) {
    speed = -MOTOR_SPEED_MAX;
  }

  uint16_t duty = (uint16_t)((abs(speed) * MOTOR_PWM_MAX) / MOTOR_SPEED_MAX);
  Motor_SetDirection(id, speed);
  Motor_SetPwm(id, duty);
}

void Motor_SetBoth(int16_t left, int16_t right)
{
  Motor_Set(MOTOR_LEFT, left);
  Motor_Set(MOTOR_RIGHT, right);
}

void Motor_StopAll(void)
{
  Motor_Set(MOTOR_LEFT, 0);
  Motor_Set(MOTOR_RIGHT, 0);
}
