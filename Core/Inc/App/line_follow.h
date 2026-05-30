/**
 * @file line_follow.h
 * @brief Thuật toán dò line PD
 */
#ifndef LINE_FOLLOW_H
#define LINE_FOLLOW_H

#include <stdint.h>

void LineFollow_Init(void);
void LineFollow_Reset(void);
void LineFollow_Update(void);
void LineFollow_Stop(void);

#endif /* LINE_FOLLOW_H */
