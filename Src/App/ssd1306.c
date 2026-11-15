#include "ssd1306.h"
#include <string.h>

static I2C_HandleTypeDef *oled_i2c = 0;
static uint8_t fb[SSD1306_H / 8][SSD1306_W];

/* ------------------------------------------------------------------
 * 5x7 font, every glyph is 7 rows of 5 pixels written as text so that
 * the table stays easy to read and edit.
 * ------------------------------------------------------------------ */
typedef struct
{
    char c;
    const char *row[7];
} glyph_t;

static const glyph_t font[] =
{
    { '0', { "01110", "10001", "10011", "10101", "11001", "10001", "01110" } },
    { '1', { "00100", "01100", "00100", "00100", "00100", "00100", "01110" } },
    { '2', { "01110", "10001", "00001", "00010", "00100", "01000", "11111" } },
    { '3', { "11111", "00010", "00100", "00010", "00001", "10001", "01110" } },
    { '4', { "00010", "00110", "01010", "10010", "11111", "00010", "00010" } },
    { '5', { "11111", "10000", "11110", "00001", "00001", "10001", "01110" } },
    { '6', { "00110", "01000", "10000", "11110", "10001", "10001", "01110" } },
    { '7', { "11111", "00001", "00010", "00100", "01000", "01000", "01000" } },
    { '8', { "01110", "10001", "10001", "01110", "10001", "10001", "01110" } },
    { '9', { "01110", "10001", "10001", "01111", "00001", "00010", "01100" } },
    { 'A', { "01110", "10001", "10001", "11111", "10001", "10001", "10001" } },
    { 'E', { "11111", "10000", "10000", "11110", "10000", "10000", "11111" } },
    { 'F', { "11111", "10000", "10000", "11110", "10000", "10000", "10000" } },
    { 'H', { "10001", "10001", "10001", "11111", "10001", "10001", "10001" } },
    { 'K', { "10001", "10010", "10100", "11000", "10100", "10010", "10001" } },
    { 'N', { "10001", "10001", "11001", "10101", "10011", "10001", "10001" } },
    { 'P', { "11110", "10001", "10001", "11110", "10000", "10000", "10000" } },
    { 'S', { "01111", "10000", "10000", "01110", "00001", "00001", "11110" } },
    { 'Z', { "11111", "00001", "00010", "00100", "01000", "10000", "11111" } },
    { 'z', { "11111", "00001", "00010", "00100", "01000", "10000", "11111" } },
    { '.', { "00000", "00000", "00000", "00000", "00000", "01100", "01100" } },
    { ':', { "00000", "01100", "01100", "00000", "01100", "01100", "00000" } },
    { '=', { "00000", "00000", "11111", "00000", "11111", "00000", "00000" } },
    { '-', { "00000", "00000", "00000", "11111", "00000", "00000", "00000" } },
    { 0,   { "00000", "00000", "00000", "00000", "00000", "00000", "00000" } }
};

/* ------------ low level I2C helpers ------------------------------- */
static void OLED_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2];
    buf[0] = 0x00;              /* Co=0, D/C=0 -> command */
    buf[1] = cmd;
    HAL_I2C_Master_Transmit(oled_i2c, SSD1306_ADDR, buf, 2, 100);
}

static void OLED_WriteData(const uint8_t *data, uint16_t len)
{
    static uint8_t buf[SSD1306_W + 1];
    if (len > SSD1306_W)
        return;
    buf[0] = 0x40;              /* Co=0, D/C=1 -> data */
    memcpy(&buf[1], data, len);
    HAL_I2C_Master_Transmit(oled_i2c, SSD1306_ADDR, buf, len + 1, 100);
}

/* ------------ public API ------------------------------------------- */
void OLED_Init(I2C_HandleTypeDef *hi2c)
{
    static const uint8_t init_cmds[] =
    {
        0xAE,             /* display off                */
        0xD5, 0x80,       /* clock divide ratio          */
        0xA8, 0x3F,       /* multiplex 1/64              */
        0xD3, 0x00,       /* display offset              */
        0x40,             /* start line 0                */
        0x8D, 0x14,       /* charge pump enable          */
        0x20, 0x02,       /* page addressing mode        */
        0xA1,             /* segment remap               */
        0xC8,             /* COM scan direction remapped */
        0xDA, 0x12,       /* COM pins                    */
        0x81, 0xCF,       /* contrast                    */
        0xD9, 0xF1,       /* pre-charge period           */
        0xDB, 0x40,       /* VCOMH                       */
        0xA4,             /* display from RAM            */
        0xA6,             /* normal (not inverted)       */
        0xAF              /* display on                  */
    };
    uint16_t i;

    oled_i2c = hi2c;
    HAL_Delay(20);

    for (i = 0; i < sizeof(init_cmds); i++)
        OLED_WriteCmd(init_cmds[i]);

    OLED_Clear();
    OLED_Refresh();
}

void OLED_Clear(void)
{
    memset(fb, 0, sizeof(fb));
}

void OLED_Pixel(int16_t x, int16_t y)
{
    if ((x < 0) || (x >= SSD1306_W) || (y < 0) || (y >= SSD1306_H))
        return;
    fb[y / 8][x] |= (uint8_t)(1 << (y % 8));
}

void OLED_Line(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while (1)
    {
        OLED_Pixel(x0, y0);
        if ((x0 == x1) && (y0 == y1))
            break;
        if ((err * 2) > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if ((err * 2) < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void OLED_HLine(int16_t x, int16_t y, int16_t w)
{
    int16_t i;
    for (i = 0; i < w; i++)
        OLED_Pixel(x + i, y);
}

void OLED_VLine(int16_t x, int16_t y, int16_t h)
{
    int16_t i;
    for (i = 0; i < h; i++)
        OLED_Pixel(x, y + i);
}

void OLED_FillRect(int16_t x, int16_t y, int16_t w, int16_t h)
{
    int16_t i, j;
    for (i = 0; i < w; i++)
        for (j = 0; j < h; j++)
            OLED_Pixel(x + i, y + j);
}

void OLED_String(int16_t x, int16_t y, const char *str)
{
    int16_t cx = x;
    uint8_t gi, row, col;

    while (*str)
    {
        uint8_t found = 0;

        for (gi = 0; font[gi].c != 0; gi++)
        {
            if (font[gi].c == *str)
            {
                found = 1;
                for (row = 0; row < 7; row++)
                {
                    const char *r = font[gi].row[row];
                    for (col = 0; col < 5; col++)
                    {
                        if (r[col] == '1')
                            OLED_Pixel(cx + col, y + row);
                    }
                }
                break;
            }
        }

        if (!found)
        {
            /* unknown character: draw a small dot so it is visible */
            OLED_Pixel(cx + 2, y + 3);
        }

        cx += 6;
        if (cx > (SSD1306_W - 6))
            break;
        str++;
    }
}

void OLED_StringNum(int16_t x, int16_t y, const char *label, float value, uint8_t decimals)
{
    char buf[20];
    int16_t i = 0;
    uint8_t d;
    uint32_t int_part, frac_part = 0;
    uint32_t mul = 1;

    while (*label && i < 10)
        buf[i++] = *label++;

    if (value < 0.0f)
    {
        buf[i++] = '-';
        value = -value;
    }

    for (d = 0; d < decimals; d++)
        mul *= 10;

    int_part = (uint32_t)value;
    if (decimals)
        frac_part = (uint32_t)((value - (float)int_part) * (float)mul + 0.5f);

    /* integer part */
    {
        char tmp[8];
        int8_t k = 0;

        if (int_part == 0)
            tmp[k++] = '0';
        while (int_part > 0 && k < 8)
        {
            tmp[k++] = (char)('0' + (int_part % 10));
            int_part /= 10;
        }
        while (k > 0)
            buf[i++] = tmp[--k];
    }

    if (decimals)
    {
        buf[i++] = '.';
        {
            char tmp[4];
            int8_t k;
            for (k = (int8_t)decimals - 1; k >= 0; k--)
            {
                tmp[k] = (char)('0' + (frac_part % 10));
                frac_part /= 10;
            }
            for (k = 0; k < (int8_t)decimals; k++)
                buf[i++] = tmp[k];
        }
    }

    buf[i] = 0;
    OLED_String(x, y, buf);
}

void OLED_Refresh(void)
{
    uint8_t page;

    for (page = 0; page < (SSD1306_H / 8); page++)
    {
        OLED_WriteCmd((uint8_t)(0xB0 | page));   /* page start address */
        OLED_WriteCmd(0x00);                     /* column low         */
        OLED_WriteCmd(0x10);                     /* column high        */
        OLED_WriteData(&fb[page][0], SSD1306_W);
    }
}
