#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <stdio.h>
#include "usart.h"      // HAL串口（CubeMX生成）
#include "cmsis_os.h"   // For osDelay

void Task1_LED(void *argument)
{
    // 第一次进来可以做一些初始化，这里不需要
    
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);  // 翻转LED
        uart_print("Task1: LED toggled\r\n");
        osDelay(500);  // 睡500ms，让出CPU
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
        
        osDelay(800);  // 睡1000ms
    }
}