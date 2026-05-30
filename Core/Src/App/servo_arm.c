/**
 * @file servo_arm.c
 */
#include "servo_arm.h"
#include "main.h"

extern TIM_HandleTypeDef htim4;

static const uint32_t s_tim_channels[SERVO_COUNT] = {
  TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3, TIM_CHANNEL_4
};

/* {BASE, VAI, KHUỶU, KẸP} — chỉnh theo robot thật */
static const uint8_t s_pose_angles[10][SERVO_COUNT] = {
  {90, 70, 110, 90},    /* HOME */
  {90, 50, 90, 130},    /* READY */
  {90, 35, 70, 60},     /* PICK - trạm hàng */
  {90, 55, 85, 60},     /* CARRY */
  {50, 50, 90, 130},    /* DROP_TRAY1 - DO */
  {90, 50, 90, 130},    /* DROP_TRAY2 - XANH */
  {130, 50, 90, 130},   /* DROP_TRAY3 - VANG */
  {90, 42, 95, 60},     /* SLOT1 - ngăn dưới (độc lập) */
  {90, 52, 82, 60},     /* SLOT2 - ngăn giữa */
  {90, 62, 72, 60},     /* SLOT3 - ngăn trên */
};

typedef struct {
  uint8_t active;
  ServoPose_t from;
  ServoPose_t to;
  uint8_t steps;
  uint8_t current_step;
  uint16_t step_ms;
  uint32_t last_tick;
} ServoSeq_t;

static ServoSeq_t s_seq;

static uint16_t ServoArm_AngleToPulse(uint8_t angle_deg)
{
  uint32_t pulse;

  if (angle_deg > 180U) {
    angle_deg = 180U;
  }
  pulse = SERVO_PULSE_MIN_US +
          ((uint32_t)angle_deg * (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US)) / 180U;
  return (uint16_t)pulse;
}

static void ServoArm_ApplyStep(uint8_t step_index)
{
  uint8_t joint;

  if (s_seq.from > SERVO_POSE_STACK_SLOT3 || s_seq.to > SERVO_POSE_STACK_SLOT3) {
    return;
  }

  for (joint = 0; joint < SERVO_COUNT; joint++) {
    int16_t a0 = (int16_t)s_pose_angles[s_seq.from][joint];
    int16_t a1 = (int16_t)s_pose_angles[s_seq.to][joint];
    int16_t a = a0 + ((a1 - a0) * (int16_t)step_index) / (int16_t)s_seq.steps;

    if (a < 0) {
      a = 0;
    } else if (a > 180) {
      a = 180;
    }
    ServoArm_SetAngle((ServoId_t)joint, (uint8_t)a);
  }
}

void ServoArm_Init(void)
{
  s_seq.active = 0U;
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
  ServoArm_GotoPose(SERVO_POSE_HOME);
}

void ServoArm_SetPulseUs(ServoId_t id, uint16_t pulse_us)
{
  if (id >= SERVO_COUNT) {
    return;
  }
  if (pulse_us < SERVO_PULSE_MIN_US) {
    pulse_us = SERVO_PULSE_MIN_US;
  } else if (pulse_us > SERVO_PULSE_MAX_US) {
    pulse_us = SERVO_PULSE_MAX_US;
  }
  __HAL_TIM_SET_COMPARE(&htim4, s_tim_channels[id], pulse_us);
}

void ServoArm_SetAngle(ServoId_t id, uint8_t angle_deg)
{
  ServoArm_SetPulseUs(id, ServoArm_AngleToPulse(angle_deg));
}

void ServoArm_GotoPose(ServoPose_t pose)
{
  uint8_t i;

  if (pose > SERVO_POSE_STACK_SLOT3) {
    return;
  }
  for (i = 0; i < SERVO_COUNT; i++) {
    ServoArm_SetAngle((ServoId_t)i, s_pose_angles[pose][i]);
  }
}

ServoPose_t ServoArm_GetDropPose(uint8_t tray_id)
{
  switch (tray_id) {
  case 1U: return SERVO_POSE_DROP_TRAY1;
  case 3U: return SERVO_POSE_DROP_TRAY3;
  default: return SERVO_POSE_DROP_TRAY2;
  }
}

ServoPose_t ServoArm_GetStackSlotPose(uint8_t slot_id)
{
  switch (slot_id) {
  case 1U: return SERVO_POSE_STACK_SLOT1;
  case 3U: return SERVO_POSE_STACK_SLOT3;
  default: return SERVO_POSE_STACK_SLOT2;
  }
}

uint8_t ServoArm_SequenceStart(ServoPose_t from, ServoPose_t to, uint8_t steps, uint16_t step_ms)
{
  if (s_seq.active) {
    return 0U;
  }
  if (from > SERVO_POSE_STACK_SLOT3 || to > SERVO_POSE_STACK_SLOT3) {
    return 0U;
  }
  if (steps < 2U) {
    steps = 2U;
  }
  if (step_ms == 0U) {
    step_ms = 20U;
  }

  s_seq.from = from;
  s_seq.to = to;
  s_seq.steps = steps;
  s_seq.step_ms = step_ms;
  s_seq.current_step = 0U;
  s_seq.last_tick = HAL_GetTick();
  s_seq.active = 1U;

  ServoArm_ApplyStep(1U);
  s_seq.current_step = 1U;
  return 1U;
}

void ServoArm_Abort(void)
{
  s_seq.active = 0U;
}

uint8_t ServoArm_IsBusy(void)
{
  return s_seq.active;
}

void ServoArm_Update(void)
{
  uint32_t now;

  if (!s_seq.active) {
    return;
  }

  now = HAL_GetTick();
  if ((now - s_seq.last_tick) < s_seq.step_ms) {
    return;
  }
  s_seq.last_tick = now;

  if (s_seq.current_step >= s_seq.steps) {
    s_seq.active = 0U;
    return;
  }

  s_seq.current_step++;
  ServoArm_ApplyStep(s_seq.current_step);

  if (s_seq.current_step >= s_seq.steps) {
    s_seq.active = 0U;
  }
}
