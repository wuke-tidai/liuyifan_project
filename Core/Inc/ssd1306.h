#ifndef __SSD1306_H
#define __SSD1306_H

#include "main.h"

/* OLED I2C 地址（通常为 0x78，若模块 DC 引脚接地则为 0x7A） */
#define SSD1306_I2C_ADDR  0x78

/* 函数声明 */
void SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);
void SSD1306_ShowNumber(uint8_t x, uint8_t y, int num, uint8_t len, uint8_t size);
void SSD1306_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t len, uint8_t size);
void SSD1306_CheckAndRecover(void);
#endif
