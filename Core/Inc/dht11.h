#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

// DHT11 引脚定义（根据你的接线修改）
#define DHT11_GPIO_Port    GPIOB
#define DHT11_Pin          GPIO_PIN_5

// 函数声明
uint8_t DHT11_Read(float *temperature, float *humidity);

#endif
