/**
 * @file cargo_stack.h
 * @brief 3 ngăn độc lập trên lưng (SLOT1..3) — lấy bất kỳ ngăn không ảnh hưởng ngăn khác
 */
#ifndef CARGO_STACK_H
#define CARGO_STACK_H

#include <stdint.h>
#include "app_config.h"

void CargoStack_Reset(void);
uint8_t CargoStack_GetCount(void);
uint8_t CargoStack_IsFull(void);
uint8_t CargoStack_IsEmpty(void);

void CargoStack_SetSlot(uint8_t slot, uint8_t color_id);
uint8_t CargoStack_IsSlotFilled(uint8_t slot);
uint8_t CargoStack_GetSlotColor(uint8_t slot);
/** Ngăn n có hàng và cánh tay có thể lấy (các ngăn độc lập) */
uint8_t CargoStack_CanAccessSlot(uint8_t slot);
void CargoStack_ClearSlot(uint8_t slot);

#endif /* CARGO_STACK_H */
