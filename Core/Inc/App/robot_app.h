/**
 * @file robot_app.h
 * @brief Điều phối chương trình chính (gọi từ main)
 */
#ifndef ROBOT_APP_H
#define ROBOT_APP_H

typedef enum {
  ROBOT_MODE_IDLE = 0,
  ROBOT_MODE_MOTOR_TEST,
  ROBOT_MODE_LINE_DEBUG,
  ROBOT_MODE_LINE_FOLLOW,
  ROBOT_MODE_SERVO_TEST,
  ROBOT_MODE_PICK_DROP,   /* thử chuỗi gắp/thả */
} RobotMode_t;

void Robot_App_Init(void);
void Robot_App_Loop(void);
void Robot_App_OnButton(void);
RobotMode_t Robot_App_GetMode(void);
void Robot_App_SetMode(RobotMode_t mode);

#endif /* ROBOT_APP_H */
