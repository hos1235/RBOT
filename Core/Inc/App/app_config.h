/**
 * @file app_config.h
 * @brief Tham số chung robot STEMCUALIEN
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

#define LINE_SENSOR_COUNT       8U
#define MOTOR_PWM_MAX           1000U   /* khớp TIM1 ARR */
#define MOTOR_SPEED_MAX         100     /* % */
#define SERVO_COUNT             4U
#define SERVO_PULSE_MIN_US      1000U
#define SERVO_PULSE_MAX_US      2000U
#define SERVO_PULSE_MID_US      1500U

#define UART_LINK_BAUD          115200U
#define UART_RX_LINE_MAX        32U

/* Line follow */
#define LINE_FOLLOW_BASE_SPEED  45
#define LINE_FOLLOW_KP          8.0f
#define LINE_FOLLOW_KD          2.0f
#define LINE_BINARY_THRESHOLD   0.5f

/* Trọng số 8 mắt: trái (-) -> phải (+) */
#define LINE_WEIGHT_0           (-7)
#define LINE_WEIGHT_1           (-5)
#define LINE_WEIGHT_2           (-3)
#define LINE_WEIGHT_3           (-1)
#define LINE_WEIGHT_4           (1)
#define LINE_WEIGHT_5           (3)
#define LINE_WEIGHT_6           (5)
#define LINE_WEIGHT_7           (7)

/* 3 loại hàng / 3 khay / 3 tầng trên lưng */
#define PRODUCT_TYPE_COUNT      3U
#define COLOR_ID_RED            1U   /* lệnh PR / DR */
#define COLOR_ID_GREEN          2U   /* PG / DG */
#define COLOR_ID_YELLOW         3U   /* PV / DV (V=Vang) */

#endif /* APP_CONFIG_H */
