#include "projecttask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <stdio.h>
#include "usart.h"    // HAL串口（CubeMX生成）
#include "cmsis_os.h" // For osDelay

#include "adxl345.h"
#include "fft.h"

// 声明外部变量（定义在其他.c文件里的）
extern I2C_HandleTypeDef hi2c1; // 在 i2c.c 里定义的

void ProjectTask_Start(void)
{
    const osThreadAttr_t sensorTaskAttr = {
        .name = "Sensor_Task",
        .priority = osPriorityAboveNormal,
        .stack_size = 2048};
    osThreadId_t sensorTaskHandle = osThreadNew(Task_Sensor, NULL, &sensorTaskAttr);
    if (sensorTaskHandle == NULL)
    {
        while (1)
        {
            uart_print("Sensor task create failed!\r\n");
            HAL_Delay(1000);
        }
    }
}

/*
 * 从单边幅值谱中提取前三个最大峰值。
 *
 * 这里从 bin 1 开始扫描，默认跳过 DC 分量，避免直流偏置干扰结果。
 * 结果按幅值从高到低排列，方便直接打印和做阈值判断。
 */
static void FFT_GetTop3Peaks(uint16_t top_bins[3], float top_magnitudes[3])
{
    uint16_t i;
    uint8_t pos;

    top_bins[0] = 0;
    top_bins[1] = 0;
    top_bins[2] = 0;
    top_magnitudes[0] = 0.0f;
    top_magnitudes[1] = 0.0f;
    top_magnitudes[2] = 0.0f;

    for (i = 1; i < (FFT_SIZE / 2); i++)
    {
        float mag = fft_magnitude[i];


        
        for (pos = 0; pos < 3; pos++)
        {
            if (mag > top_magnitudes[pos])
            {
                uint8_t shift;

                for (shift = 2; shift > pos; shift--)
                {
                    top_magnitudes[shift] = top_magnitudes[shift - 1u];
                    top_bins[shift] = top_bins[shift - 1u];
                }

                top_magnitudes[pos] = mag;
                top_bins[pos] = i;
                break;
            }
        }
    }
}

// ADXL345 传感器读取任务
void Task_Sensor(void *argument)
{
    char buf[160]; /* 打印缓冲区，要够大 */
    uint8_t ret;
    ADXL345_Raw raw;
    uint32_t i;
    uint16_t top_bins[3];
    float top_magnitudes[3];
    float top_freqs[3];

    /* ---- ① 初始化传感器（只执行一次）---- */
    ret = ADXL345_Init(&hi2c1);
    if (ret != 0)
    {
        uart_print("[Sensor] ADXL345 Init FAILED\r\n");
        for (;;)
            osDelay(1000); /* 死循环等待 */
    }
    uart_print("[Sensor] ADXL345 Init OK\r\n");

    /* ---- ② 初始化FFT（只执行一次）---- */
    ret = FFT_Init();
    if (ret != 0)
    {
        uart_print("[Sensor] FFT Init FAILED\r\n");
        for (;;)
            osDelay(1000);
    }
    uart_print("[Sensor] FFT Ready, N=256, Fs=2000Hz\r\n");

    /* ---- 主循环：采样→FFT→输出 ---- */
    for (;;)
    {

        /* ===== 第A步: 高速采集256个点 =====
         * 目标采样率约2000Hz → 每隔0.5ms采一个点
         * 实际用轮询+osDelay精度有限，但足够演示
         * 真正的高精度采样应该用定时器触发ADC+DMA
         */
        for (i = 0; i < FFT_SIZE; i++)
        {
            ret = ADXL345_ReadRaw(&hi2c1, &raw);
            if (ret == 0)
            {
                /* 取X轴作为振动分析通道（也可以做三路FFT） */
                fft_input[i] = (float)raw.x / 256.0f; /* 转为g值 */
            }
            else
            {
                fft_input[i] = 0.0f; /* 读失败填0 */
            }
            /* 短延时控制采样间隔
             * 2000Hz = 每0.5ms一个点
             * 但I2C读取本身就要几ms，实际达不到2000Hz
             * 这里先跑通逻辑，后面优化采样方式
             */
            osDelay(1);
        }

        /* ===== 第B步: 执行FFT ===== */
        FFT_Execute();
        FFT_GetTop3Peaks(top_bins, top_magnitudes);
        top_freqs[0] = (float)top_bins[0] * FREQ_RES;
        top_freqs[1] = (float)top_bins[1] * FREQ_RES;
        top_freqs[2] = (float)top_bins[2] * FREQ_RES;

        /* ===== 第C步: 打印结果 ===== */
        sprintf(buf, "Top 3 peaks:\r\n");
        uart_print(buf);

        sprintf(buf, "  #1 %.1fHz (%.3fg)\r\n", top_freqs[0], top_magnitudes[0]);
        uart_print(buf);

        sprintf(buf, "  #2 %.1fHz (%.3fg)\r\n", top_freqs[1], top_magnitudes[1]);
        uart_print(buf);

        sprintf(buf, "  #3 %.1fHz (%.3fg)\r\n", top_freqs[2], top_magnitudes[2]);
        uart_print(buf);

        /* ===== 第D步: 报警判断（阈值暂定0.5g）==== */
        if (top_magnitudes[0] > 0.5f)
        {
            uart_print("[ALERT] Vibration Exceeded!\r\n");
            /* 后面接继电器控制 */
        }

        osDelay(500); /* 每轮结束后暂停500ms再开始下一轮 */
    }
}