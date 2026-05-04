#include "fft.h"
#include "usart.h"
#include <stdio.h>
#include <math.h>

/* ========== 缓冲区定义 ========== */
/*
 * fft_input:
 *   外部先把时域采样写入这里，FFT 执行时直接读取。
 * fft_output:
 *   中间计算和最终频域结果都放在这里，采用复数交错布局。
 * fft_magnitude:
 *   归一化后的单边幅值谱，便于直接查找主频峰值。
 */
float fft_input[FFT_SIZE];
float fft_output[FFT_SIZE * 2];
float fft_magnitude[FFT_SIZE / 2];

static uint16_t bitrev_table[FFT_SIZE];
static uint8_t fft_initialized = 0;

static uint8_t is_power_of_two(uint16_t x)
{
    /* 经典判定：n > 0 且仅有一位为 1 时，x 才是 2 的幂。 */
    return (x != 0u) && ((x & (x - 1u)) == 0u);
}

static uint16_t reverse_bits(uint16_t x, uint8_t bits)
{
    uint16_t r = 0;
    uint8_t i;

    /* 按 bit 位逐位翻转，用于构建位反转索引表。 */
    for (i = 0; i < bits; i++)
    {
        r = (uint16_t)((r << 1) | (x & 1u));
        x >>= 1;
    }

    return r;
}

/*
 * FFT 初始化（只需调用一次）。
 *
 * 这里不做任何采样或频谱计算，只负责检查配置并预生成位反转表。
 * 位反转表可以避免每次执行 FFT 时重复计算索引，加快后续处理。
 *
 * 返回：0=成功，1=失败。
 */
uint8_t FFT_Init(void)
{
    uint16_t i;
    uint16_t n = FFT_SIZE;
    uint8_t bits = 0;

    if (!is_power_of_two(n))
    {
        return 1;
    }

    while (n > 1u)
    {
        bits++;
        n >>= 1;
    }

    for (i = 0; i < FFT_SIZE; i++)
    {
        bitrev_table[i] = reverse_bits(i, bits);
    }

    fft_initialized = 1;
    return 0;
}

/*
 * 执行完整 FFT 流程。
 *
 * 流程分为三段：
 *   1) 按位反转顺序重排输入样本；
 *   2) 从 2 点蝶形开始逐级合并，直到得到完整频谱；
 *   3) 计算单边幅值谱，并返回最大峰值所在的 bin。
 */
uint16_t FFT_Execute(void)
{
    uint16_t i;
    uint16_t k;
    uint16_t step;
    const float pi = 3.14159265358979323846f;

    if (!fft_initialized)
    {
        return 0;
    }

    for (i = 0; i < FFT_SIZE; i++)
    {
        uint16_t j = bitrev_table[i];
        /* 先把输入重排到位反转顺序，方便原地完成迭代 FFT。 */
        fft_output[2u * j] = fft_input[i];
        fft_output[2u * j + 1u] = 0.0f;
    }

    /* 逐级蝶形运算：step 表示当前分组长度，2 -> 4 -> 8 -> ... -> FFT_SIZE。 */
    for (step = 2; step <= FFT_SIZE; step <<= 1)
    {
        uint16_t half = (uint16_t)(step >> 1);
        float angle_base = -2.0f * pi / (float)step;
        uint16_t m;

        /* 同一组内，不同蝶形位置对应不同旋转因子。 */
        for (m = 0; m < half; m++)
        {
            float wr = cosf(angle_base * (float)m);
            float wi = sinf(angle_base * (float)m);
            uint16_t p;

            /* 以 step 为间隔遍历当前层的所有蝶形单元。 */
            for (p = m; p < FFT_SIZE; p = (uint16_t)(p + step))
            {
                uint16_t q = (uint16_t)(p + half);
                float ur = fft_output[2u * p];
                float ui = fft_output[2u * p + 1u];
                float vr = fft_output[2u * q];
                float vi = fft_output[2u * q + 1u];
                float tr = wr * vr - wi * vi;
                float ti = wr * vi + wi * vr;

                fft_output[2u * p] = ur + tr;
                fft_output[2u * p + 1u] = ui + ti;
                fft_output[2u * q] = ur - tr;
                fft_output[2u * q + 1u] = ui - ti;
            }
        }
    }

    /*
     * 将复数频谱转换成幅值谱，并做单边幅值归一化。
     * 对于实数输入，正负频率镜像，因此只保留前半部分。
     */
    for (k = 0; k < FFT_SIZE / 2; k++)
    {
        float real = fft_output[2u * k];
        float imag = fft_output[2u * k + 1u];
        fft_magnitude[k] = (2.0f / (float)FFT_SIZE) * sqrtf(real * real + imag * imag);
    }

    /* DC 分量不做双边谱补偿，避免它的幅值被错误放大。 */
    fft_magnitude[0] *= 0.5f;

    /* 扫描单边幅值谱，找出主峰所在的 bin。 */
    float max_mag = 0.0f;
    uint16_t max_idx = 0;

    for (i = 1; i < FFT_SIZE / 2; i++)
    {
        if (fft_magnitude[i] > max_mag)
        {
            max_mag = fft_magnitude[i];
            max_idx = (uint16_t)i;
        }
    }

    return max_idx;
}
