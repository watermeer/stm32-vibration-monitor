#ifndef FFT_H
#define FFT_H

#include <stdint.h>

/* ========== FFT参数配置 ========== */
/*
 * FFT_SIZE:
 *   固定为 256 点，必须是 2 的幂，便于使用迭代蝶形算法。
 * SAMPLE_RATE:
 *   时域采样频率，后续可据此把 bin 索引换算成实际频率。
 * FREQ_RES:
 *   单个 bin 对应的频率间隔，用于把频谱索引转换为 Hz。
 */
#define FFT_SIZE 256
#define SAMPLE_RATE 2000
#define FREQ_RES ((float)SAMPLE_RATE / FFT_SIZE)

/* ========== 缓冲区声明（外部可访问） ========== */
/*
 * fft_input:
 *   外部写入的时域采样数据，长度为 FFT_SIZE。
 * fft_output:
 *   复数频域结果，按“实部、虚部、实部、虚部...”交错存储。
 * fft_magnitude:
 *   单边幅值谱，只保存 0 ~ FFT_SIZE/2-1 的正频率部分。
 */
extern float fft_input[FFT_SIZE];
extern float fft_output[FFT_SIZE * 2];
extern float fft_magnitude[FFT_SIZE / 2];

/* ========== 函数声明 ========== */
/*
 * 初始化 FFT 模块，只需调用一次。
 *
 * 主要工作：
 *   1) 检查 FFT_SIZE 是否为 2 的幂；
 *   2) 预计算位反转表，供后续重排输入使用。
 *
 * 返回值：0=成功，1=失败。
 */
uint8_t FFT_Init(void);

/*
 * 执行完整 FFT 流程：
 *   1) 将时域输入按位反转顺序重排；
 *   2) 进行逐级蝶形运算，得到频域复数结果；
 *   3) 计算单边幅值谱并写入 fft_magnitude[]；
 *   4) 扫描幅值谱，返回最大峰值所在的 bin 索引。
 *
 * 返回值：最大幅值所在的 bin 索引（0 ~ FFT_SIZE/2-1）。
 * 实际频率 = 返回值 * FREQ_RES。
 */
uint16_t FFT_Execute(void);

#endif /* FFT_H */
