#ifndef __SSD1306_H
#define __SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* most 0.96" I2C OLED modules use address 0x3C, some use 0x3D */
#define SSD1306_ADDR   (0x3C << 1)

#define SSD1306_W      128
#define SSD1306_H      64

void    OLED_Init(I2C_HandleTypeDef *hi2c);
void    OLED_Clear(void);
void    OLED_Refresh(void);
void    OLED_Pixel(int16_t x, int16_t y);
void    OLED_Line(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void    OLED_HLine(int16_t x, int16_t y, int16_t w);
void    OLED_VLine(int16_t x, int16_t y, int16_t h);
void    OLED_FillRect(int16_t x, int16_t y, int16_t w, int16_t h);
void    OLED_String(int16_t x, int16_t y, const char *str);
void    OLED_StringNum(int16_t x, int16_t y, const char *label, float value, uint8_t decimals);

#ifdef __cplusplus
}
#endif

#endif /* __SSD1306_H */
