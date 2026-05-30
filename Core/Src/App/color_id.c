/**
 * @file color_id.c
 */
#include "color_id.h"
#include <stddef.h>
#include <string.h>

static char Color_ToUpper(char c)
{
  if (c >= 'a' && c <= 'z') {
    return (char)(c - 'a' + 'A');
  }
  return c;
}

uint8_t Color_IdFromLetter(char letter)
{
  switch (Color_ToUpper(letter)) {
  case 'R':
    return COLOR_ID_RED;
  case 'G':
    return COLOR_ID_GREEN;
  case 'V':
  case 'Y':
    return COLOR_ID_YELLOW;
  default:
    return 0U;
  }
}

uint8_t Color_IdFromName(const char *line)
{
  if (line == NULL || line[0] == '\0') {
    return 0U;
  }
  if (strncmp(line, "RED", 3) == 0) {
    return COLOR_ID_RED;
  }
  if (strncmp(line, "GREEN", 5) == 0 || strncmp(line, "XANH", 4) == 0) {
    return COLOR_ID_GREEN;
  }
  if (strncmp(line, "YELLOW", 6) == 0 || strncmp(line, "VANG", 4) == 0) {
    return COLOR_ID_YELLOW;
  }
  return Color_IdFromLetter(line[0]);
}

const char *Color_Name(uint8_t color_id)
{
  switch (color_id) {
  case COLOR_ID_RED:
    return "RED";
  case COLOR_ID_GREEN:
    return "GREEN";
  case COLOR_ID_YELLOW:
    return "YELLOW";
  default:
    return "?";
  }
}

uint8_t Color_TrayFromColor(uint8_t color_id)
{
  return color_id;
}
