#ifndef __FFT_ANALYZER_H
#define __FFT_ANALYZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define FFT_N        1024          /* must be a power of 2                      */
#define FFT_HALF     (FFT_N / 2)   /* useful spectrum points (0 ... Fs/2)       */
#define FFT_FS       1000.0f       /* sampling rate in Hz (1ms per sample)      */

/* 4 point moving average: cheapest low pass, removes high frequency noise */
void FFT_MovingAverage(float *data, uint32_t n, uint32_t win);

/* remove DC offset (mean value) from the block */
float FFT_RemoveDC(float *data, uint32_t n);

/* apply a Hann window, reduces spectrum leakage */
void FFT_HannWindow(float *data, uint32_t n);

/* real FFT with the official ARM CMSIS-DSP library (arm_rfft_fast_f32):
 * mag[0..n/2] = amplitude spectrum, the input block is used as scratch    */
void FFT_Spectrum(float *data, float *mag, uint32_t n);

/* return the index of the biggest bin inside [bin_low, bin_high] */
uint32_t FFT_PeakBin(const float *mag, uint32_t bin_low, uint32_t bin_high);

#ifdef __cplusplus
}
#endif

#endif /* __FFT_ANALYZER_H */
