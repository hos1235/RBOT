/**
 * @file robot_app.c
 */
#include "robot_app.h"
#include "motor.h"
#include "line_sensor.h"
#include "line_follow.h"
#include "servo_arm.h"
#include "uart_link.h"
#include "cargo_stack.h"
#include "color_id.h"
#include "main.h"
#include "app_config.h"

#ifndef ROBOT_DEFAULT_MODE
#define ROBOT_DEFAULT_MODE  ROBOT_MODE_LINE_DEBUG
#endif

typedef enum {
  ARM_TASK_NONE = 0,
  ARM_TASK_LOAD_STACK,    /* trạm lấy: PICK -> STACK_SLOT */
  ARM_TASK_LIFT_SLOT,     /* U1/U2/U3: lấy tầng chỉ định -> CARRY */
  ARM_TASK_DROP_TRAY,     /* DR/DG/DV: thả vào khay (đang cầm) */
  ARM_TASK_DEMO_PICK_DROP,
} ArmTask_t;

typedef enum {
  ARM_PHASE_IDLE = 0,
  ARM_PHASE_LOAD_SEQ_PICK,
  ARM_PHASE_LOAD_SEQ_TO_SLOT,
  ARM_PHASE_UNLOAD_SEQ_FROM_SLOT,
  ARM_PHASE_UNLOAD_SEQ_DROP,
  ARM_PHASE_GOTO_HOME,
  ARM_PHASE_DEMO_SEQ1,
  ARM_PHASE_DEMO_SEQ2,
} ArmPhase_t;

static RobotMode_t s_mode;
static uint32_t s_last_debug_ms;
static uint8_t s_motor_test_phase;
static uint32_t s_motor_test_ms;

static ArmTask_t s_arm_task;
static ArmPhase_t s_arm_phase;
static uint8_t s_pick_drop_started;
static uint8_t s_load_color;
static uint8_t s_load_slot;
static uint8_t s_unload_tray;
static uint8_t s_lift_slot;
static uint8_t s_carry_color;
static uint8_t s_arm_carrying;

static const char *Robot_ModeName(RobotMode_t mode)
{
  switch (mode) {
  case ROBOT_MODE_IDLE:         return "IDLE";
  case ROBOT_MODE_MOTOR_TEST:   return "MOTOR_TEST";
  case ROBOT_MODE_LINE_DEBUG:   return "LINE_DEBUG";
  case ROBOT_MODE_LINE_FOLLOW:  return "LINE_FOLLOW";
  case ROBOT_MODE_SERVO_TEST:   return "SERVO_TEST";
  case ROBOT_MODE_PICK_DROP:    return "PICK_DROP";
  default:                      return "?";
  }
}

static void Robot_LED_Toggle(void)
{
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}

static uint8_t Robot_ClampColorId(uint8_t color_id)
{
  if (color_id < COLOR_ID_RED || color_id > COLOR_ID_YELLOW) {
    return 0U;
  }
  return color_id;
}

static void Robot_EmergencyStop(void)
{
  LineFollow_Stop();
  Motor_StopAll();
  ServoArm_Abort();
  s_arm_task = ARM_TASK_NONE;
  s_arm_phase = ARM_PHASE_IDLE;
  CargoStack_Reset();
  s_load_color = 0U;
  s_load_slot = 0U;
  s_unload_tray = 0U;
  s_lift_slot = 0U;
  s_carry_color = 0U;
  s_arm_carrying = 0U;
}

/**
 * Trạm lấy hàng: gắp 1 hộp (P1 đỏ / P2 xanh / P3 vàng) xếp lên tầng trên lưng.
 */
static void Robot_ArmTask_StartLoadStack(uint8_t color_id)
{
  uint8_t slot;

  if (CargoStack_IsFull()) {
    UartLink_Print("STACK FULL\r\n");
    return;
  }

  color_id = Robot_ClampColorId(color_id);
  if (color_id == 0U) {
    UartLink_Print("USE PR PG PV\r\n");
    return;
  }

  slot = (uint8_t)(CargoStack_GetCount() + 1U);
  s_load_color = color_id;
  s_load_slot = slot;
  s_arm_task = ARM_TASK_LOAD_STACK;
  s_arm_phase = ARM_PHASE_LOAD_SEQ_PICK;
  LineFollow_Stop();
  Motor_StopAll();

  if (!ServoArm_SequenceStart(SERVO_POSE_READY, SERVO_POSE_PICK, 25U, 20U)) {
    s_arm_task = ARM_TASK_NONE;
    UartLink_Print("LOAD BUSY\r\n");
  }
}

/**
 * Bước 1 — Nhìn khay / quyết định: hạ hộp từ tầng U1/U2/U3 (không thả ngay).
 */
static void Robot_ArmTask_StartLiftFromSlot(uint8_t slot)
{
  uint8_t color;

  if (s_arm_carrying) {
    UartLink_Print("ARM BUSY drop first\r\n");
    return;
  }
  if (slot < 1U || slot > PRODUCT_TYPE_COUNT) {
    UartLink_Print("USE U1 U2 U3\r\n");
    return;
  }
  if (!CargoStack_IsSlotFilled(slot)) {
    UartLink_Print("SLOT EMPTY\r\n");
    return;
  }

  color = CargoStack_GetSlotColor(slot);
  s_lift_slot = slot;
  s_carry_color = color;
  s_arm_task = ARM_TASK_LIFT_SLOT;
  s_arm_phase = ARM_PHASE_UNLOAD_SEQ_FROM_SLOT;
  LineFollow_Stop();
  Motor_StopAll();

  if (!ServoArm_SequenceStart(ServoArm_GetStackSlotPose(slot), SERVO_POSE_CARRY, 25U, 20U)) {
    s_arm_task = ARM_TASK_NONE;
    UartLink_Print("LIFT BUSY\r\n");
  }
}

/**
 * Bước 2 — Sau khi đã cầm: thả vào khay DR/DG/DV (theo màu khay người sắp).
 */
static void Robot_ArmTask_StartDropToTray(uint8_t tray_color_id)
{
  uint8_t tray_id;

  if (!s_arm_carrying) {
    UartLink_Print("NEED U1 U2 U3 first\r\n");
    return;
  }

  tray_id = Robot_ClampColorId(tray_color_id);
  if (tray_id == 0U) {
    UartLink_Print("USE DR DG DV\r\n");
    return;
  }

  s_unload_tray = Color_TrayFromColor(tray_id);
  s_arm_task = ARM_TASK_DROP_TRAY;
  s_arm_phase = ARM_PHASE_UNLOAD_SEQ_DROP;
  LineFollow_Stop();
  Motor_StopAll();

  if (!ServoArm_SequenceStart(SERVO_POSE_CARRY, ServoArm_GetDropPose(s_unload_tray), 25U, 20U)) {
    s_arm_task = ARM_TASK_NONE;
    UartLink_Print("DROP BUSY\r\n");
  }
}

static void Robot_ArmTask_StartDemo(void)
{
  s_arm_task = ARM_TASK_DEMO_PICK_DROP;
  s_arm_phase = ARM_PHASE_DEMO_SEQ1;
  LineFollow_Stop();
  Motor_StopAll();
  if (!ServoArm_SequenceStart(SERVO_POSE_HOME, SERVO_POSE_PICK, 30U, 25U)) {
    s_arm_task = ARM_TASK_NONE;
  }
}

static void Robot_ArmTask_Update(void)
{
  if (s_arm_task == ARM_TASK_NONE || ServoArm_IsBusy()) {
    return;
  }

  switch (s_arm_task) {
  case ARM_TASK_LOAD_STACK:
    if (s_arm_phase == ARM_PHASE_LOAD_SEQ_PICK) {
      s_arm_phase = ARM_PHASE_LOAD_SEQ_TO_SLOT;
      if (!ServoArm_SequenceStart(SERVO_POSE_PICK, ServoArm_GetStackSlotPose(s_load_slot), 25U, 20U)) {
        s_arm_task = ARM_TASK_NONE;
      }
    } else if (s_arm_phase == ARM_PHASE_LOAD_SEQ_TO_SLOT) {
      CargoStack_SetSlot(s_load_slot, s_load_color);
      ServoArm_GotoPose(SERVO_POSE_HOME);
      UartLink_Printf("LOAD %s SLOT%u OK (%u/3)\r\n",
                      Color_Name(s_load_color), s_load_slot, CargoStack_GetCount());
      s_arm_task = ARM_TASK_NONE;
      s_arm_phase = ARM_PHASE_IDLE;
    }
    break;

  case ARM_TASK_LIFT_SLOT:
    s_arm_carrying = 1U;
    UartLink_Printf("LIFT %s FROM SLOT%u (send DR/DG/DV)\r\n",
                    Color_Name(s_carry_color), s_lift_slot);
    s_arm_task = ARM_TASK_NONE;
    s_arm_phase = ARM_PHASE_IDLE;
    break;

  case ARM_TASK_DROP_TRAY:
    if (s_arm_phase == ARM_PHASE_UNLOAD_SEQ_DROP) {
      CargoStack_ClearSlot(s_lift_slot);
      ServoArm_GotoPose(SERVO_POSE_HOME);
      UartLink_Printf("DROP %s -> TRAY %s SLOT%u cleared (%u left)\r\n",
                      Color_Name(s_carry_color), Color_Name(s_unload_tray),
                      s_lift_slot, CargoStack_GetCount());
      s_arm_carrying = 0U;
      s_carry_color = 0U;
      s_lift_slot = 0U;
      s_arm_task = ARM_TASK_NONE;
      s_arm_phase = ARM_PHASE_IDLE;
    }
    break;

  case ARM_TASK_DEMO_PICK_DROP:
    if (s_arm_phase == ARM_PHASE_DEMO_SEQ1) {
      s_arm_phase = ARM_PHASE_DEMO_SEQ2;
      if (!ServoArm_SequenceStart(SERVO_POSE_PICK, SERVO_POSE_DROP_TRAY2, 30U, 25U)) {
        s_arm_task = ARM_TASK_NONE;
        s_arm_phase = ARM_PHASE_IDLE;
      }
    } else if (s_arm_phase == ARM_PHASE_DEMO_SEQ2) {
      ServoArm_GotoPose(SERVO_POSE_HOME);
      s_arm_task = ARM_TASK_NONE;
      s_arm_phase = ARM_PHASE_IDLE;
    }
    break;

  default:
    s_arm_task = ARM_TASK_NONE;
    s_arm_phase = ARM_PHASE_IDLE;
    break;
  }
}

static void Robot_HandleUartCommand(void)
{
  UartCmd_t cmd = UartLink_GetCommand();

  if (cmd == UART_CMD_NONE) {
    return;
  }

  switch (cmd) {
  case UART_CMD_STOP:
    Robot_EmergencyStop();
    UartLink_Print("STOP\r\n");
    break;
  case UART_CMD_GO:
    if (s_mode == ROBOT_MODE_IDLE) {
      Robot_App_SetMode(ROBOT_MODE_LINE_FOLLOW);
    }
    break;
  case UART_CMD_CAL_W:
    LineSensor_CalibrateWhite();
    UartLink_Print("CAL WHITE OK\r\n");
    break;
  case UART_CMD_CAL_B:
    LineSensor_CalibrateBlack();
    UartLink_Print("CAL BLACK OK\r\n");
    break;
  case UART_CMD_PICK:
    Robot_ArmTask_StartLoadStack(UartLink_GetCommandArg());
    break;
  case UART_CMD_UNLOAD_SLOT:
    Robot_ArmTask_StartLiftFromSlot(UartLink_GetCommandArg());
    break;
  case UART_CMD_DROP:
    Robot_ArmTask_StartDropToTray(UartLink_GetCommandArg());
    break;
  case UART_CMD_CLEAR_STACK:
    CargoStack_Reset();
    UartLink_Print("STACK CLEAR\r\n");
    break;
  case UART_CMD_STACK_STATUS:
    UartLink_Printf("STACK %u/3 S1=%s S2=%s S3=%s carry=%u\r\n",
                    CargoStack_GetCount(),
                    CargoStack_IsSlotFilled(1U) ? Color_Name(CargoStack_GetSlotColor(1U)) : "-",
                    CargoStack_IsSlotFilled(2U) ? Color_Name(CargoStack_GetSlotColor(2U)) : "-",
                    CargoStack_IsSlotFilled(3U) ? Color_Name(CargoStack_GetSlotColor(3U)) : "-",
                    s_arm_carrying);
    break;
  default:
    break;
  }

  UartLink_ClearCommand();
}

static void Robot_ModeLineDebug(void)
{
  const uint16_t *raw = LineSensor_GetRaw();
  uint8_t i;

  if ((HAL_GetTick() - s_last_debug_ms) < 100U) {
    return;
  }
  s_last_debug_ms = HAL_GetTick();

  UartLink_Printf("L:");
  for (i = 0; i < LINE_SENSOR_COUNT; i++) {
    UartLink_Printf("%s%u", (i > 0U) ? "," : "", raw[i]);
  }
  UartLink_Printf(" E:%.2f\r\n", LineSensor_GetError());
}

static void Robot_ModeMotorTest(void)
{
  if ((HAL_GetTick() - s_motor_test_ms) < 1500U) {
    return;
  }
  s_motor_test_ms = HAL_GetTick();

  switch (s_motor_test_phase) {
  case 0:
    Motor_SetBoth(40, 40);
    break;
  case 1:
    Motor_StopAll();
    break;
  case 2:
    Motor_SetBoth(-40, 40);
    break;
  default:
    Motor_StopAll();
    s_motor_test_phase = 0U;
    return;
  }
  s_motor_test_phase++;
}

static void Robot_ModeServoTest(void)
{
  static uint8_t step;
  static uint32_t t;

  if (ServoArm_IsBusy() || (s_arm_task != ARM_TASK_NONE)) {
    return;
  }

  if ((HAL_GetTick() - t) < 2000U) {
    return;
  }
  t = HAL_GetTick();

  switch (step) {
  case 0:
    ServoArm_GotoPose(SERVO_POSE_PICK);
    break;
  case 1:
    ServoArm_GotoPose(SERVO_POSE_STACK_SLOT1);
    break;
  case 2:
    ServoArm_GotoPose(SERVO_POSE_STACK_SLOT2);
    break;
  case 3:
    ServoArm_GotoPose(SERVO_POSE_STACK_SLOT3);
    break;
  default:
    ServoArm_GotoPose(SERVO_POSE_HOME);
    step = 0U;
    return;
  }
  step++;
}

void Robot_App_Init(void)
{
  s_mode = ROBOT_DEFAULT_MODE;
  s_last_debug_ms = 0U;
  s_motor_test_phase = 0U;
  s_motor_test_ms = 0U;
  s_arm_task = ARM_TASK_NONE;
  s_arm_phase = ARM_PHASE_IDLE;
  s_pick_drop_started = 0U;
  CargoStack_Reset();

  UartLink_Init();
  Motor_Init();
  LineSensor_Init();
  LineFollow_Init();
  ServoArm_Init();

  UartLink_Printf("STEMCUALIEN ready. Mode=%s\r\n", Robot_ModeName(s_mode));
  UartLink_Print("PR PG PV load | U1-U3 lift tier | DR DG DV drop | ? status\r\n");
}

void Robot_App_SetMode(RobotMode_t mode)
{
  if (mode == s_mode) {
    return;
  }

  Robot_EmergencyStop();
  s_mode = mode;
  s_motor_test_phase = 0U;
  s_pick_drop_started = 0U;
  UartLink_Printf("Mode=%s\r\n", Robot_ModeName(s_mode));
}

RobotMode_t Robot_App_GetMode(void)
{
  return s_mode;
}

void Robot_App_OnButton(void)
{
  RobotMode_t next = (RobotMode_t)(((uint8_t)s_mode + 1U) % ((uint8_t)ROBOT_MODE_PICK_DROP + 1U));
  Robot_App_SetMode(next);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == NUT_TEST_Pin) {
    Robot_App_OnButton();
  }
}

void Robot_App_Loop(void)
{
  static uint32_t led_ms;

  /* UART trước — lệnh S được xử lý ngay, kể cả khi servo đang chạy */
  UartLink_Process();
  Robot_HandleUartCommand();

  ServoArm_Update();
  Robot_ArmTask_Update();

  switch (s_mode) {
  case ROBOT_MODE_IDLE:
    if (s_arm_task == ARM_TASK_NONE) {
      Motor_StopAll();
    }
    break;

  case ROBOT_MODE_MOTOR_TEST:
    if (s_arm_task == ARM_TASK_NONE) {
      Robot_ModeMotorTest();
    }
    break;

  case ROBOT_MODE_LINE_DEBUG:
    Robot_ModeLineDebug();
    break;

  case ROBOT_MODE_LINE_FOLLOW:
    if (s_arm_task == ARM_TASK_NONE) {
      LineFollow_Update();
    } else {
      Motor_StopAll();
    }
    break;

  case ROBOT_MODE_SERVO_TEST:
    Robot_ModeServoTest();
    break;

  case ROBOT_MODE_PICK_DROP:
    if (!s_pick_drop_started && s_arm_task == ARM_TASK_NONE) {
      s_pick_drop_started = 1U;
      Robot_ArmTask_StartDemo();
    }
    break;

  default:
    break;
  }

  if ((HAL_GetTick() - led_ms) >= 500U) {
    led_ms = HAL_GetTick();
    Robot_LED_Toggle();
  }
}
