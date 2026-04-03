#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <stdio.h>
#include "usart.h"    // HAL串口（CubeMX生成）
#include "cmsis_os.h" // For osDelay

#include "adxl345.h"

// 声明外部变量（定义在其他.c文件里的）
extern I2C_HandleTypeDef hi2c1; // 在 i2c.c 里定义的

// ADXL345 传感器读取任务
void Task_Sensor(void *argument)
{
    char buf[64];
    ADXL345_Raw raw;

    // 等系统稳定
    osDelay(500);

    // 初始化
    uint8_t ret = ADXL345_Init(&hi2c1);
    if (ret != 0)
    {
        while (1)
        {
            sprintf(buf, "ADXL345 Init FAILED! Error=%d\r\n", ret);
            uart_print(buf);
            osDelay(1000);
        }
    }
    uart_print("ADXL345 Init OK!\r\n");

    while (1)
    {
        if (ADXL345_ReadRaw(&hi2c1, &raw) == 0)
        {
            // ±2g全分辨率：4mg/LSB = 0.004g/LSB
            // 为避免浮点打印问题，直接打印原始值
            sprintf(buf, "X:%d Y:%d Z:%d\r\n", raw.x, raw.y, raw.z);
            uart_print(buf);
        }
        else
        {
            uart_print("Read Error!\r\n");
        }

        osDelay(100); // 100ms读一次
    }
}

void Task1_LED(void *argument)
{
    // 第一次进来可以做一些初始化，这里不需要

    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13); // 翻转LED
        uart_print("Task1: LED toggled\r\n");
        osDelay(500); // 睡500ms，让出CPU
    }
}

// 任务2：计数打印
void Task2_Print(void *argument)
{
    uint32_t count = 0;
    char buf[48];

    while (1)
    {
        count++;
        snprintf(buf, sizeof(buf), "Task2: count = %lu\r\n", (unsigned long)count);
        uart_print(buf);

        osDelay(800); // 睡800ms
    }
}