/**
 * @file motor.h
 * @brief Điều khiển 2 motor qua TB6612 (TIM1 PWM + GPIO DIR)
 */
#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

typedef enum {
  MOTOR_LEFT = 0,
  MOTOR_RIGHT = 1
} MotorId_t;

void Motor_Init(void);
void Motor_Set(MotorId_t id, int16_t speed_percent);
void Motor_SetBoth(int16_t left, int16_t right);
void Motor_StopAll(void);

#endif /* MOTOR_H */
