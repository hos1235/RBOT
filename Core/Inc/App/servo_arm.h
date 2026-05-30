/**
 * @file servo_arm.h
 * @brief Cánh tay 4 servo — trạm lấy, 3 tầng trên lưng, 3 khay thả
 */
#ifndef SERVO_ARM_H
#define SERVO_ARM_H

#include <stdint.h>
#include "app_config.h"

typedef enum {
  SERVO_BASE = 0,
  SERVO_SHOULDER,
  SERVO_ELBOW,
  SERVO_GRIP,
} ServoId_t;

typedef enum {
  SERVO_POSE_HOME = 0,
  SERVO_POSE_READY,
  SERVO_POSE_PICK,          /* lấy từ trạm hàng */
  SERVO_POSE_CARRY,         /* mang giữa các điểm */
  SERVO_POSE_DROP_TRAY1,   /* khay DO */
  SERVO_POSE_DROP_TRAY2,   /* khay XANH */
  SERVO_POSE_DROP_TRAY3,   /* khay VANG */
  SERVO_POSE_STACK_SLOT1,   /* đặt / lấy tầng 1 (thấp) trên lưng */
  SERVO_POSE_STACK_SLOT2,
  SERVO_POSE_STACK_SLOT3,   /* tầng 3 (cao) */
  SERVO_POSE_DROP = SERVO_POSE_DROP_TRAY2,
} ServoPose_t;

ServoPose_t ServoArm_GetDropPose(uint8_t tray_id);
ServoPose_t ServoArm_GetStackSlotPose(uint8_t slot_id);

void ServoArm_Init(void);
void ServoArm_Update(void);
void ServoArm_SetPulseUs(ServoId_t id, uint16_t pulse_us);
void ServoArm_SetAngle(ServoId_t id, uint8_t angle_deg);
void ServoArm_GotoPose(ServoPose_t pose);
uint8_t ServoArm_SequenceStart(ServoPose_t from, ServoPose_t to, uint8_t steps, uint16_t step_ms);
uint8_t ServoArm_IsBusy(void);
void ServoArm_Abort(void);

#endif /* SERVO_ARM_H */
