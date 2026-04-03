#include "adxl345.h"

/*
 * 初始化 ADXL345
 *
 * 返回值：0=成功，1=I2C通信失败，2=设备ID不对
 */
uint8_t ADXL345_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t val = 0;

    // 第1步：读设备ID，应该是0xE5
    if (HAL_I2C_Mem_Read(hi2c, ADXL345_ADDR, ADXL345_DEVID,
                         I2C_MEMADD_SIZE_8BIT, &val, 1, 100) != HAL_OK) {
        return 1;  // I2C通信失败，检查接线
    }
    if (val != 0xE5) {
        return 2;  // ID不对，检查CS和SDO是否接对
    }

    // 第2步：设置采样率 100Hz
    val = 0x0A;  // 100Hz
    HAL_I2C_Mem_Write(hi2c, ADXL345_ADDR, ADXL345_BW_RATE,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, 100);

    // 第3步：设置量程 ±2g，全分辨率模式
    val = 0x08;  // bit3=1 全分辨率，bit1:0=00 → ±2g，灵敏度4mg/LSB
    HAL_I2C_Mem_Write(hi2c, ADXL345_ADDR, ADXL345_DATA_FORMAT,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, 100);

    // 第4步：启动测量（POWER_CTL的bit3=Measure置1）
    val = 0x08;
    HAL_I2C_Mem_Write(hi2c, ADXL345_ADDR, ADXL345_POWER_CTL,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, 100);

    return 0;
}

/*
 * 读取三轴原始数据
 *
 * 从0x32开始连续读6个字节：
 * X低 X高 Y低 Y高 Z低 Z高
 */
uint8_t ADXL345_ReadRaw(I2C_HandleTypeDef *hi2c, ADXL345_Raw *data)
{
    uint8_t buf[6];

    if (HAL_I2C_Mem_Read(hi2c, ADXL345_ADDR, ADXL345_DATAX0,
                         I2C_MEMADD_SIZE_8BIT, buf, 6, 100) != HAL_OK) {
        return 1;
    }

    // 高低字节拼成16位有符号整数
    data->x = (int16_t)((buf[1] << 8) | buf[0]);
    data->y = (int16_t)((buf[3] << 8) | buf[2]);
    data->z = (int16_t)((buf[5] << 8) | buf[4]);

    return 0;
}
