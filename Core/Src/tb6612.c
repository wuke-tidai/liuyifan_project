#include "tb6612.h"
#include "tim.h"

void TB6612_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = TB6612_AIN1_Pin;
    HAL_GPIO_Init(TB6612_AIN1_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = TB6612_AIN2_Pin;
    HAL_GPIO_Init(TB6612_AIN2_GPIO_Port, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = TB6612_STBY_Pin;
    HAL_GPIO_Init(TB6612_STBY_GPIO_Port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(TB6612_STBY_GPIO_Port, TB6612_STBY_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
}

void TB6612_SetSpeed(uint16_t pwm_value)
{
    if (pwm_value > 999) pwm_value = 999;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm_value);
}

void TB6612_SetDirection(uint8_t dir)
{
    if (dir == 1) {
        HAL_GPIO_WritePin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin, GPIO_PIN_RESET);
    } else if (dir == 0) {
        HAL_GPIO_WritePin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin, GPIO_PIN_SET);
    } else {
        TB6612_Stop();
    }
}

void TB6612_Stop(void)
{
    HAL_GPIO_WritePin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin, GPIO_PIN_RESET);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
}
