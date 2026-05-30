/**
 * @file uart_link.c
 * @brief PR PG PV | U1-U3 hạ tầng | DR DG DV thả khay | G S W B C ?
 */
#include "uart_link.h"
#include "color_id.h"
#include "app_config.h"
#include "main.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

static uint8_t s_rx_byte;
static char s_line_buf[UART_RX_LINE_MAX];
static uint8_t s_line_len;
static volatile UartCmd_t s_pending_cmd;
static volatile uint8_t s_pending_arg;

static uint8_t UartLink_ParseColorAfterPrefix(char prefix, const char *line)
{
  if (line[0] != prefix && line[0] != (char)(prefix + 32)) {
    return 0U;
  }
  if (line[1] != '\0' && line[2] == '\0') {
    return Color_IdFromLetter(line[1]);
  }
  return Color_IdFromName(&line[1]);
}

static void UartLink_ParseLine(char *line)
{
  uint8_t color_id;

  if (line[0] == '\0') {
    return;
  }

  color_id = UartLink_ParseColorAfterPrefix('P', line);
  if (color_id != 0U) {
    s_pending_cmd = UART_CMD_PICK;
    s_pending_arg = color_id;
    return;
  }

  color_id = UartLink_ParseColorAfterPrefix('D', line);
  if (color_id != 0U) {
    s_pending_cmd = UART_CMD_DROP;
    s_pending_arg = color_id;
    return;
  }

  if ((line[0] == 'U' || line[0] == 'u') && line[1] >= '1' && line[1] <= '3' && line[2] == '\0') {
    s_pending_cmd = UART_CMD_UNLOAD_SLOT;
    s_pending_arg = (uint8_t)(line[1] - '0');
    return;
  }

  switch (line[0]) {
  case 'G':
  case 'g':
    s_pending_cmd = UART_CMD_GO;
    s_pending_arg = 0U;
    break;
  case 'S':
  case 's':
    s_pending_cmd = UART_CMD_STOP;
    s_pending_arg = 0U;
    break;
  case 'W':
  case 'w':
    s_pending_cmd = UART_CMD_CAL_W;
    s_pending_arg = 0U;
    break;
  case 'B':
  case 'b':
    s_pending_cmd = UART_CMD_CAL_B;
    s_pending_arg = 0U;
    break;
  case 'C':
  case 'c':
    s_pending_cmd = UART_CMD_CLEAR_STACK;
    s_pending_arg = 0U;
    break;
  case '?':
    s_pending_cmd = UART_CMD_STACK_STATUS;
    s_pending_arg = 0U;
    break;
  default:
    color_id = Color_IdFromName(line);
    if (color_id != 0U) {
      s_pending_cmd = UART_CMD_DROP;
      s_pending_arg = color_id;
    }
    break;
  }
}

static void UartLink_OnByte(uint8_t b)
{
  if (b == '\r') {
    return;
  }
  if (b == '\n') {
    s_line_buf[s_line_len] = '\0';
    UartLink_ParseLine(s_line_buf);
    s_line_len = 0U;
    return;
  }
  if (s_line_len < (UART_RX_LINE_MAX - 1U)) {
    s_line_buf[s_line_len++] = (char)b;
  }
}

void UartLink_Init(void)
{
  s_line_len = 0U;
  s_pending_cmd = UART_CMD_NONE;
  s_pending_arg = 0U;
  HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U);
}

void UartLink_Process(void)
{
}

UartCmd_t UartLink_GetCommand(void)
{
  return s_pending_cmd;
}

uint8_t UartLink_GetCommandArg(void)
{
  return s_pending_arg;
}

void UartLink_ClearCommand(void)
{
  s_pending_cmd = UART_CMD_NONE;
  s_pending_arg = 0U;
}

void UartLink_Print(const char *s)
{
  if (s != NULL) {
    HAL_UART_Transmit(&huart1, (uint8_t *)s, (uint16_t)strlen(s), 100U);
  }
}

void UartLink_PrintRaw(const uint8_t *data, uint16_t len)
{
  if ((data != NULL) && (len > 0U)) {
    HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 100U);
  }
}

void UartLink_Printf(const char *fmt, ...)
{
  char buf[128];
  va_list args;
  int n;

  va_start(args, fmt);
  n = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (n > 0) {
    HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)n, 200U);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1) {
    UartLink_OnByte(s_rx_byte);
    HAL_UART_Receive_IT(&huart1, &s_rx_byte, 1U);
  }
}
