#include "dht11.h"
#include "stm32f1xx_hal.h"

// 微秒延时（基于 DWT，需要使能 DWT）
static void DWT_Delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

// 使能 DWT 计数器
void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

// 设置 DHT11 引脚为输出
static void DHT11_Set_Pin_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}

// 设置 DHT11 引脚为输入
static void DHT11_Set_Pin_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}

// 读取 DHT11 数据，返回 0 表示成功，1 表示失败
uint8_t DHT11_Read(float *temperature, float *humidity)
{
    uint8_t data[5] = {0};
    uint8_t i, j;

    // 确保 DWT 已初始化
    DWT_Init();

    // 主机发送开始信号：拉低至少 18ms
    DHT11_Set_Pin_Output();
    HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET);
    DWT_Delay_us(20000); // 20ms
    HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET);
    DWT_Delay_us(30);    // 30us

    // 切换到输入，等待 DHT11 响应
    DHT11_Set_Pin_Input();

    // 等待 DHT11 拉低 80us
    uint32_t timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET) {
        if (++timeout > 1000) return 1; // 超时
        DWT_Delay_us(1);
    }
    // 等待 DHT11 拉高 80us
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_RESET) {
        if (++timeout > 1000) return 1;
        DWT_Delay_us(1);
    }
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET) {
        if (++timeout > 1000) return 1;
        DWT_Delay_us(1);
    }

    // 读取 40 位数据
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            // 等待低电平结束
            timeout = 0;
            while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_RESET) {
                if (++timeout > 1000) return 1;
                DWT_Delay_us(1);
            }
            // 延时 40us 后判断高电平持续时间
            DWT_Delay_us(40);
            if (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET) {
                data[i] |= (1 << (7 - j)); // 高电平长，表示 1
                // 等待高电平结束
                timeout = 0;
                while (HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin) == GPIO_PIN_SET) {
                    if (++timeout > 1000) return 1;
                    DWT_Delay_us(1);
                }
            }
        }
    }

    // 校验
    if ((data[0] + data[1] + data[2] + data[3]) != data[4]) {
        return 1; // 校验失败
    }

    // 解析数据
    *humidity = (float)data[0] + (float)data[1] / 10.0f;
    *temperature = (float)data[2] + (float)data[3] / 10.0f;

    return 0;
}
