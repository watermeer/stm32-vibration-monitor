#ifndef __ADXL345_H
#define __ADXL345_H

#include "main.h"

// 设备地址（7位地址0x53，HAL接口需要左移1位后的8位地址）
#define ADXL345_ADDR (0x53 << 1)

// 寄存器地址（ADXL345内部的一排"抽屉"，每个抽屉存一种数据）
#define ADXL345_DEVID 0x00       // 设备ID，固定是0xE5，用来验证通信
#define ADXL345_BW_RATE 0x2C     // 采样率设置
#define ADXL345_POWER_CTL 0x2D   // 电源控制（启动/停止测量）
#define ADXL345_DATA_FORMAT 0x31 // 量程和分辨率设置
#define ADXL345_DATAX0 0x32      // X轴数据低字节（从这里连续读6字节）

// 三轴原始数据
typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} ADXL345_Raw;

// 初始化
// 返回值：0=成功，1=I2C通信失败，2=设备ID不对
uint8_t ADXL345_Init(I2C_HandleTypeDef *hi2c);

// 读取原始数据
uint8_t ADXL345_ReadRaw(I2C_HandleTypeDef *hi2c, ADXL345_Raw *data);

#endif
