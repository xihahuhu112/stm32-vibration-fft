#include "fft_analyzer.h"
#include <math.h>
#include "arm_math.h"

/* CMSIS-DSP real FFT instance and output buffer, created on first use */
static arm_rfft_fast_instance_f32 rfft_inst;
static float32_t rfft_out[FFT_N];
static uint8_t   rfft_inited = 0;

/*
 * Centred moving average: data[i] = mean(tmp[i-half ... i+half]),
 * edges are clamped.  It is a cheap low pass that removes the high
 * frequency noise riding on the raw accelerometer samples.
 */
void FFT_MovingAverage(float *data, uint32_t n, uint32_t win)
{
    static float tmp[FFT_N];
    uint32_t i, j;
    uint32_t half;
    float sum;

    if ((win == 0) || (win > n) || (n > FFT_N))
        return;

    half = win / 2;

    for (i = 0; i < n; i++)
        tmp[i] = data[i];

    for (i = 0; i < n; i++)
    {
        sum = 0.0f;
        for (j = 0; j < win; j++)
        {
            int32_t idx = (int32_t)i + (int32_t)j - (int32_t)half;

            if (idx < 0)
                idx = 0;
            if (idx >= (int32_t)n)
                idx = (int32_t)n - 1;

            sum += tmp[idx];
        }
        data[i] = sum / (float)win;
    }
}

float FFT_RemoveDC(float *data, uint32_t n)
{
    uint32_t i;
    float sum = 0.0f;
    float mean;

    for (i = 0; i < n; i++)
        sum += data[i];

    mean = sum / (float)n;

    for (i = 0; i < n; i++)
        data[i] -= mean;

    return mean;
}

void FFT_HannWindow(float *data, uint32_t n)
{
    uint32_t i;

    for (i = 0; i < n; i++)
    {
        float w = 0.5f - 0.5f * cosf(2.0f * 3.14159265358979f * (float)i / (float)(n - 1));
        data[i] *= w;
    }
}

/*
 * Real FFT of the official ARM CMSIS-DSP library.
 * arm_rfft_fast_f32 packs the result as:
 *   out[0]             : DC component
 *   out[1]             : Nyquist component
 *   out[2k], out[2k+1] : real / imaginary part of bin k (k = 1 ... n/2-1)
 */
void FFT_Spectrum(float *data, float *mag, uint32_t n)
{
    uint32_t k;
    float32_t re, im;
    float scale;

    if (!rfft_inited)
    {
        arm_rfft_fast_init_f32(&rfft_inst, n);
        rfft_inited = 1;
    }

    arm_rfft_fast_f32(&rfft_inst, data, rfft_out, 0);   /* 0 = forward transform */

    scale = 1.0f / (float)n;

    mag[0]     = fabsf(rfft_out[0]) * scale;            /* DC      */
    mag[n / 2] = fabsf(rfft_out[1]) * scale;            /* Nyquist */

    for (k = 1; k < (n / 2); k++)
    {
        re = rfft_out[2 * k];
        im = rfft_out[2 * k + 1];
        mag[k] = sqrtf(re * re + im * im) * 2.0f * scale;
    }
}

uint32_t FFT_PeakBin(const float *mag, uint32_t bin_low, uint32_t bin_high)
{
    uint32_t i;
    uint32_t idx = bin_low;
    float max = 0.0f;

    for (i = bin_low; i <= bin_high; i++)
    {
        if (mag[i] > max)
        {
            max = mag[i];
            idx = i;
        }
    }

    return idx;
}
