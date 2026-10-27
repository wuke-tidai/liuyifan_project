#ifndef __TB6612_H
#define __TB6612_H

#include "main.h"

#define TB6612_AIN1_GPIO_Port   GPIOB
#define TB6612_AIN1_Pin         GPIO_PIN_12
#define TB6612_AIN2_GPIO_Port   GPIOB
#define TB6612_AIN2_Pin         GPIO_PIN_13
#define TB6612_STBY_GPIO_Port   GPIOB
#define TB6612_STBY_Pin         GPIO_PIN_14

void TB6612_Init(void);
void TB6612_SetSpeed(uint16_t pwm_value);
void TB6612_SetDirection(uint8_t dir); // 1=正转, 0=反转, 2=停止
void TB6612_Stop(void);

#endif
