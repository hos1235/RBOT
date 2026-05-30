/**
 * @file uart_link.h
 */
#ifndef UART_LINK_H
#define UART_LINK_H

#include <stdint.h>

typedef enum {
  UART_CMD_NONE = 0,
  UART_CMD_GO,
  UART_CMD_STOP,
  UART_CMD_PICK,         /* PR PG PV — xếp lên lưng */
  UART_CMD_UNLOAD_SLOT,  /* U1 U2 U3 — hạ tầng chỉ định xuống (đang cầm) */
  UART_CMD_DROP,         /* DR DG DV — thả vào khay màu */
  UART_CMD_CAL_W,
  UART_CMD_CAL_B,
  UART_CMD_CLEAR_STACK,
  UART_CMD_STACK_STATUS,
} UartCmd_t;

void UartLink_Init(void);
void UartLink_Process(void);
UartCmd_t UartLink_GetCommand(void);
uint8_t UartLink_GetCommandArg(void);
void UartLink_ClearCommand(void);

void UartLink_Print(const char *s);
void UartLink_PrintRaw(const uint8_t *data, uint16_t len);
void UartLink_Printf(const char *fmt, ...);

#endif /* UART_LINK_H */
