/**
 * @file cargo_stack.c
 */
#include "cargo_stack.h"
#include <stddef.h>

static uint8_t s_slot_color[PRODUCT_TYPE_COUNT];

static uint8_t CargoStack_Index(uint8_t slot)
{
  if (slot < 1U || slot > PRODUCT_TYPE_COUNT) {
    return 0xFFU;
  }
  return (uint8_t)(slot - 1U);
}

void CargoStack_Reset(void)
{
  uint8_t i;

  for (i = 0U; i < PRODUCT_TYPE_COUNT; i++) {
    s_slot_color[i] = 0U;
  }
}

uint8_t CargoStack_GetCount(void)
{
  uint8_t i;
  uint8_t n = 0U;

  for (i = 0U; i < PRODUCT_TYPE_COUNT; i++) {
    if (s_slot_color[i] != 0U) {
      n++;
    }
  }
  return n;
}

uint8_t CargoStack_IsFull(void)
{
  return (CargoStack_GetCount() >= PRODUCT_TYPE_COUNT) ? 1U : 0U;
}

uint8_t CargoStack_IsEmpty(void)
{
  return (CargoStack_GetCount() == 0U) ? 1U : 0U;
}

void CargoStack_SetSlot(uint8_t slot, uint8_t color_id)
{
  uint8_t idx = CargoStack_Index(slot);

  if (idx == 0xFFU) {
    return;
  }
  s_slot_color[idx] = color_id;
}

uint8_t CargoStack_IsSlotFilled(uint8_t slot)
{
  uint8_t idx = CargoStack_Index(slot);

  if (idx == 0xFFU) {
    return 0U;
  }
  return (s_slot_color[idx] != 0U) ? 1U : 0U;
}

uint8_t CargoStack_GetSlotColor(uint8_t slot)
{
  uint8_t idx = CargoStack_Index(slot);

  if (idx == 0xFFU) {
    return 0U;
  }
  return s_slot_color[idx];
}

uint8_t CargoStack_CanAccessSlot(uint8_t slot)
{
  /* 3 ngăn tách rời — không cần tháo tầng trên trước */
  return CargoStack_IsSlotFilled(slot);
}

void CargoStack_ClearSlot(uint8_t slot)
{
  uint8_t idx = CargoStack_Index(slot);

  if (idx == 0xFFU) {
    return;
  }
  s_slot_color[idx] = 0U;
}
