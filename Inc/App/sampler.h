#ifndef __SAMPLER_H
#define __SAMPLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "fft_analyzer.h"

/*
 * TIM3 driven sampler: one accelerometer sample every 1ms (1kHz),
 * the conversion is done inside the timer update interrupt, so the
 * sampling rate is exactly the timer period instead of a software loop.
 *
 * TIM3 sits on APB1 (42MHz), the timer clock is doubled -> 84MHz,
 * prescaler 84 -> 1MHz counter, period 1000 -> 1kHz update interrupt.
 */
void   Sampler_Init(I2C_HandleTypeDef *hi2c);
void   Sampler_Start(void);
uint8_t Sampler_IsDone(void);
float *Sampler_Buffer(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAMPLER_H */
