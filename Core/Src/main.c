/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dht11.h"
#include "ssd1306.h"
#include "tb6612.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t uart_rx_byte = 0;
volatile uint8_t uart_rx_flag = 0;   // 收到一字节标志
char cmdBuf[16];
uint8_t cmdIdx = 0;
volatile uint8_t cmd_ready = 0;      // 收到完整命令标志
uint16_t adcValues[2];   // ADC 双通道数据
QueueHandle_t displayQueue;
QueueHandle_t controlQueue;
volatile uint8_t auto_mode = 1; 
// 传感器数据结构
typedef struct {
    float temp;
    float humi;
	 uint16_t light;
    uint16_t thermistor;
} SensorData_t;

// 全局状态
volatile uint8_t uart_rx_char = 0;   // 接收到的字符
volatile uint8_t uart_rx_ready = 0;  // 收到标志
volatile uint8_t fan_enable = 1;      // 风扇使能，默认开启
volatile uint16_t fan_speed = 0;      // 当前风扇速度 0~999
volatile int16_t encoder_counter = 0;  // 编码器计数值
volatile uint32_t last_enc_time = 0;   // 编码器消抖时间
uint16_t adcValues[2];   // ADC 双通道数据缓冲区
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
static void SensorTask(void *arg);
static void DisplayTask(void *arg);
static void ControlTask(void *arg);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
   // 初始化 OLED
  SSD1306_Init();
  SSD1306_Clear();
  SSD1306_ShowString(0, 0, "Starting", 1);

  // 初始化 TB6612 和 PWM
  TB6612_Init();
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcValues, 2);
  HAL_UART_Receive_IT(&huart1, (uint8_t*)&uart_rx_char, 1);
  // 创建队列
  displayQueue = xQueueCreate(5, sizeof(SensorData_t));
  controlQueue = xQueueCreate(5, sizeof(SensorData_t));

  

  // 创建任务
  xTaskCreate(SensorTask, "sensor", 512, NULL, 2, NULL);
  xTaskCreate(DisplayTask, "display", 512, NULL, 1, NULL);
  xTaskCreate(ControlTask, "control", 256, NULL, 3, NULL);

  
 
  // 启动调度器
  osKernelStart();

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1) {
        uart_rx_ready = 1;                              // 置标志
        HAL_UART_Receive_IT(&huart1, (uint8_t*)&uart_rx_char, 1); // 重新开启接收
    }
    if (huart->Instance == USART1) {
        uint8_t c = uart_rx_byte;

        if (c == '\n') {
            cmdBuf[cmdIdx] = '\0';
            cmdIdx = 0;
            cmd_ready = 1;
        } else {
            if (cmdIdx < 15) {
                cmdBuf[cmdIdx++] = c;
            } else {
                cmdIdx = 0;  // 超长则丢弃
            }
        }

        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    }
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {  // 编码器 A 相
        uint32_t now = HAL_GetTick();
        if (now - last_enc_time < 20) {  // 20ms 内忽略，强消抖
            return;
        }
        last_enc_time = now;

        // 延迟 1ms 等信号稳定
        HAL_Delay(1);  // 注意：在中断中 HAL_Delay 可能不精确，但可用简单循环代替

        // 读取 B 相电平判断方向
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET) {
            encoder_counter++;
        } else {
            encoder_counter--;
        }
    }
}


// 传感器任务：读取 DHT11，发送到两个队列
static void SensorTask(void *arg)
{
    SensorData_t data;
    for (;;) {
        vTaskSuspendAll();
        uint8_t ok = DHT11_Read(&data.temp, &data.humi);
        xTaskResumeAll();

        if (ok != 0) {
            data.temp = 0.0f;
            data.humi = 0.0f;
        }

        // 直接读取 ADC 数据（DMA 循环模式自动更新）
        data.light = adcValues[0];      // PA0 光敏
        data.thermistor = adcValues[1]; // PA1 热敏

        xQueueSend(displayQueue, &data, 0);
        xQueueSend(controlQueue, &data, 0);
		//串口发数据
		char uartBuf[64];
		snprintf(uartBuf, sizeof(uartBuf), "T:%.1f,H:%.1f,L:%d,TH:%d\n",
				 data.temp, data.humi, data.light, data.thermistor);
		HAL_UART_Transmit(&huart1, (uint8_t*)uartBuf, strlen(uartBuf), 100);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 显示任务：显示温湿度和风扇状态
static void DisplayTask(void *arg)
{
    SensorData_t data;
    char buf[32];
    for (;;) {
		 SSD1306_CheckAndRecover();
        if (xQueueReceive(displayQueue, &data, pdMS_TO_TICKS(200)) == pdTRUE) {
            SSD1306_Clear();
            snprintf(buf, sizeof(buf), "T:%.1f C", data.temp);
            SSD1306_ShowString(0, 0, buf, 1);
            snprintf(buf, sizeof(buf), "H:%.1f %%", data.humi);
            SSD1306_ShowString(0, 1, buf, 1);
			snprintf(buf, sizeof(buf), "L:%d", data.light);
            SSD1306_ShowString(0, 4, buf, 1);
            snprintf(buf, sizeof(buf), "Th:%d", data.thermistor);
            SSD1306_ShowString(0, 5, buf, 1);

            if (fan_speed > 0) {
                snprintf(buf, sizeof(buf), "Fan:%d%%", (fan_speed * 100) / 999);
            } else {
                snprintf(buf, sizeof(buf), "Fan:OFF");
            }
            SSD1306_ShowString(0, 2, buf, 1);

            if (auto_mode) {
                snprintf(buf, sizeof(buf), "Mode:Auto");
            } else {
                snprintf(buf, sizeof(buf), "Mode:Manual");
            }
            SSD1306_ShowString(0, 3, buf, 1);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// 控制任务：根据温度自动调节风扇
static void ControlTask(void *arg)
{
    SensorData_t data;
    static int16_t last_encoder = 0;
    uint16_t pwm = 0;
    static uint8_t key1_prev = 1;
    uint8_t key1_now;
    static uint8_t lastA = 1;
    uint8_t nowA;

    for (;;)
    {
        // ========== 1. 处理 ESP32 远程命令 ==========
        if (uart_rx_ready) {
            uart_rx_ready = 0;
            if (uart_rx_char == '1') {
                auto_mode = !auto_mode;              // 切换模式
                last_encoder = encoder_counter;
            } else if (uart_rx_char == '2') {
                fan_enable = !fan_enable;            // 切换风扇开关
                // 立即生效：关闭时直接停，打开时不用做，下一轮会自动控制
                if (!fan_enable) {
                    TB6612_Stop();
                    fan_speed = 0;
                }
            }
        }

        // ========== 2. 编码器轮询 ==========
        nowA = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
        if (lastA == 1 && nowA == 0) {
            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET) {
                encoder_counter--;
            } else {
                encoder_counter++;
            }
        }
        lastA = nowA;

        // ========== 3. 按键检测 ==========
        key1_now = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
        if (key1_now == GPIO_PIN_RESET) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        } else {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        }

        if (key1_prev == 1 && key1_now == 0) {
            vTaskDelay(pdMS_TO_TICKS(20));
            if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == 0) {
                auto_mode = !auto_mode;
                last_encoder = encoder_counter;
            }
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
        key1_prev = key1_now;

        // ========== 4. 接收传感器数据 ==========
        BaseType_t gotData = xQueueReceive(controlQueue, &data, 0);

        // ========== 5. 蜂鸣器报警 ==========
        if (gotData == pdTRUE) {
            if (data.temp > 30.0f) {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
            } else {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
            }
        }

        // ========== 6. 自动模式：根据温度调速（关键修改：加 fan_enable 判断） ==========
        if (auto_mode && gotData == pdTRUE) {
            if (fan_enable) {                          // ← 只有使能时才控制风扇
                if (data.temp > 30.0f) {
                    pwm = 999;
                } else if (data.temp > 25.0f) {
                    pwm = (uint16_t)((data.temp - 25.0f) * (999 / 5.0f));
                } else {
                    pwm = 0;
                }

                if (pwm > 0) {
                    TB6612_SetDirection(1);
                    TB6612_SetSpeed(pwm);
                } else {
                    TB6612_Stop();
                }
                fan_speed = pwm;
            } else {
                // 风扇被禁用，保持停止
                TB6612_Stop();
                fan_speed = 0;
            }
        }

        // ========== 7. 手动模式：编码器调速（关键修改：加 fan_enable 判断） ==========
        if (!auto_mode) {
            if (fan_enable) {                          // ← 只有使能时才允许编码器调速
                if (encoder_counter != last_encoder) {
                    int16_t delta = (int16_t)(encoder_counter - last_encoder);
                    last_encoder = encoder_counter;

                    int32_t temp_pwm = (int32_t)pwm + (int32_t)delta * 50;
                    if (temp_pwm > 999) temp_pwm = 999;
                    else if (temp_pwm < 0) temp_pwm = 0;
                    pwm = (uint16_t)temp_pwm;

                    if (pwm > 0) {
                        TB6612_SetDirection(1);
                        TB6612_SetSpeed(pwm);
                    } else {
                        TB6612_Stop();
                    }
                    fan_speed = pwm;
                }
            } else {
                // 风扇被禁用，保持停止
                TB6612_Stop();
                fan_speed = 0;
            }
        }

        // ========== 8. 清空队列残留 ==========
        while (xQueueReceive(controlQueue, &data, 0) == pdTRUE) {}

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
