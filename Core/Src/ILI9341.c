#include "ILI9341.h"
#include "stm32f4xx_hal_spi.h"

/* vim: set ai et ts=4 sw=4: */
#include "stm32f4xx_hal.h"

#include <string.h>

extern SPI_HandleTypeDef hspi2;

/* TFT pins */
#define TFT_CS_PORT     GPIOB
#define TFT_CS_PIN      GPIO_PIN_12

#define TFT_DC_PORT     GPIOB
#define TFT_DC_PIN      GPIO_PIN_10

#define TFT_RST_PORT    GPIOB
#define TFT_RST_PIN     GPIO_PIN_2


/* ---------------- PIN CONTROL ---------------- */

static void TFT_CS_Low(void)
{
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_RESET);
}

static void TFT_CS_High(void)
{
    HAL_GPIO_WritePin(TFT_CS_PORT, TFT_CS_PIN, GPIO_PIN_SET);
}

static void TFT_DC_Command(void)
{
    HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_RESET);
}

static void TFT_DC_Data(void)
{
    HAL_GPIO_WritePin(TFT_DC_PORT, TFT_DC_PIN, GPIO_PIN_SET);
}


/* ---------------- COMMAND ---------------- */

static void TFT_WriteCommand(uint8_t cmd)
{
    TFT_CS_Low();
    TFT_DC_Command();

    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);

    TFT_CS_High();
}


/* ---------------- DATA ---------------- */

static void TFT_WriteData(uint8_t *data, uint16_t size)
{
    TFT_CS_Low();
    TFT_DC_Data();

    HAL_SPI_Transmit(&hspi2, data, size, HAL_MAX_DELAY);

    TFT_CS_High();
}


/* ---------------- ADDRESS WINDOW ---------------- */

static void TFT_SetAddressWindow(uint16_t x0, uint16_t y0,
                                 uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    /* Column address */
    TFT_WriteCommand(0x2A);

    data[0] = x0 >> 8;
    data[1] = x0 & 0xFF;
    data[2] = x1 >> 8;
    data[3] = x1 & 0xFF;

    TFT_WriteData(data, 4);


    /* Page address */
    TFT_WriteCommand(0x2B);

    data[0] = y0 >> 8;
    data[1] = y0 & 0xFF;
    data[2] = y1 >> 8;
    data[3] = y1 & 0xFF;

    TFT_WriteData(data, 4);


    /* Memory write */
    TFT_WriteCommand(0x2C);
}


/* ---------------- INIT ---------------- */

void ILI9341_Init(void)
{
    /* Hardware reset */
    HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(10);

    HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);

    HAL_GPIO_WritePin(TFT_RST_PORT, TFT_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(120);


    /* Software reset */
    TFT_WriteCommand(0x01);
    HAL_Delay(120);


    /* Pixel format: 16-bit RGB565 */
    TFT_WriteCommand(0x3A);
    uint8_t data = 0x55;
    TFT_WriteData(&data, 1);


    /* Landscape orientation */
    TFT_WriteCommand(0x36);
    data = 0x28;
    TFT_WriteData(&data, 1);


    /* Frame rate */
    TFT_WriteCommand(0xB1);

    uint8_t frame[] = {0x00, 0x18};
    TFT_WriteData(frame, 2);


    /* Display function control */
    TFT_WriteCommand(0xB6);

    uint8_t display[] = {0x08, 0x82, 0x27};
    TFT_WriteData(display, 3);


    /* Power control */
    TFT_WriteCommand(0xC0);
    data = 0x23;
    TFT_WriteData(&data, 1);

    TFT_WriteCommand(0xC1);
    data = 0x10;
    TFT_WriteData(&data, 1);


    /* VCOM */
    TFT_WriteCommand(0xC5);

    uint8_t vcom[] = {0x3E, 0x28};
    TFT_WriteData(vcom, 2);


    TFT_WriteCommand(0xC7);
    data = 0x86;
    TFT_WriteData(&data, 1);


    /* Gamma */
    TFT_WriteCommand(0xE0);

    uint8_t gamma1[] =
    {
        0x0F, 0x31, 0x2B, 0x0C,
        0x0E, 0x08, 0x4E, 0xF1,
        0x37, 0x07, 0x10, 0x03,
        0x0E, 0x09, 0x00
    };

    TFT_WriteData(gamma1, 15);


    TFT_WriteCommand(0xE1);

    uint8_t gamma2[] =
    {
        0x00, 0x0E, 0x14, 0x03,
        0x11, 0x07, 0x31, 0xC1,
        0x48, 0x08, 0x0F, 0x0C,
        0x31, 0x36, 0x0F
    };

    TFT_WriteData(gamma2, 15);


    /* Sleep out */
    TFT_WriteCommand(0x11);
    HAL_Delay(120);


    /* Display ON */
    TFT_WriteCommand(0x29);
    HAL_Delay(20);


    ILI9341_FillScreen(BLACK);
}


/* ---------------- DRAW PIXEL ---------------- */

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT)
        return;

    TFT_SetAddressWindow(x, y, x, y);

    uint8_t data[2];

    data[0] = color >> 8;
    data[1] = color & 0xFF;

    TFT_WriteData(data, 2);
}


/* ---------------- FILL RECTANGLE ---------------- */

void ILI9341_FillRect(uint16_t x, uint16_t y,
                      uint16_t w, uint16_t h,
                      uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT)
        return;

    if (x + w > TFT_WIDTH)
        w = TFT_WIDTH - x;

    if (y + h > TFT_HEIGHT)
        h = TFT_HEIGHT - y;

    TFT_SetAddressWindow(x, y,
                         x + w - 1,
                         y + h - 1);

    uint8_t data[2];

    data[0] = color >> 8;
    data[1] = color & 0xFF;

    TFT_CS_Low();
    TFT_DC_Data();

    uint32_t pixels = (uint32_t)w * h;

    for (uint32_t i = 0; i < pixels; i++)
    {
        HAL_SPI_Transmit(&hspi2, data, 2, HAL_MAX_DELAY);
    }

    TFT_CS_High();
}


/* ---------------- FILL SCREEN ---------------- */

void ILI9341_FillScreen(uint16_t color)
{
    ILI9341_FillRect(0, 0,
                     TFT_WIDTH,
                     TFT_HEIGHT,
                     color);
}


/* ---------------- FONT ---------------- */

static const uint8_t font5x7[][5] =
{
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}  /* 9 */
};


static void GetGlyph(char c, uint8_t glyph[5])
{
    memset(glyph, 0, 5);

    if (c >= '0' && c <= '9')
    {
        memcpy(glyph, font5x7[c - '0'], 5);
        return;
    }

    switch(c)
    {
        case 'A':
            glyph[0]=0x7E; glyph[1]=0x11; glyph[2]=0x11;
            glyph[3]=0x11; glyph[4]=0x7E;
            break;

        case 'B':
            glyph[0]=0x7F; glyph[1]=0x49; glyph[2]=0x49;
            glyph[3]=0x49; glyph[4]=0x36;
            break;

        case 'C':
            glyph[0]=0x3E; glyph[1]=0x41; glyph[2]=0x41;
            glyph[3]=0x41; glyph[4]=0x22;
            break;

        case 'D':
            glyph[0]=0x7F; glyph[1]=0x41; glyph[2]=0x41;
            glyph[3]=0x22; glyph[4]=0x1C;
            break;

        case 'E':
            glyph[0]=0x7F; glyph[1]=0x49; glyph[2]=0x49;
            glyph[3]=0x49; glyph[4]=0x41;
            break;

        case 'F':
            glyph[0]=0x7F; glyph[1]=0x09; glyph[2]=0x09;
            glyph[3]=0x09; glyph[4]=0x01;
            break;

        case 'H':
            glyph[0]=0x7F; glyph[1]=0x08; glyph[2]=0x08;
            glyph[3]=0x08; glyph[4]=0x7F;
            break;

        case 'I':
            glyph[0]=0x00; glyph[1]=0x41; glyph[2]=0x7F;
            glyph[3]=0x41; glyph[4]=0x00;
            break;

        case 'K':
            glyph[0]=0x7F; glyph[1]=0x08; glyph[2]=0x14;
            glyph[3]=0x22; glyph[4]=0x41;
            break;

        case 'M':
            glyph[0]=0x7F; glyph[1]=0x02; glyph[2]=0x0C;
            glyph[3]=0x02; glyph[4]=0x7F;
            break;

        case 'N':
            glyph[0]=0x7F; glyph[1]=0x04; glyph[2]=0x08;
            glyph[3]=0x10; glyph[4]=0x7F;
            break;

        case 'O':
            glyph[0]=0x3E; glyph[1]=0x41; glyph[2]=0x41;
            glyph[3]=0x41; glyph[4]=0x3E;
            break;

        case 'P':
            glyph[0]=0x7F; glyph[1]=0x09; glyph[2]=0x09;
            glyph[3]=0x09; glyph[4]=0x06;
            break;

        case 'R':
            glyph[0]=0x7F; glyph[1]=0x09; glyph[2]=0x19;
            glyph[3]=0x29; glyph[4]=0x46;
            break;

        case 'S':
            glyph[0]=0x46; glyph[1]=0x49; glyph[2]=0x49;
            glyph[3]=0x49; glyph[4]=0x31;
            break;

        case 'T':
            glyph[0]=0x01; glyph[1]=0x01; glyph[2]=0x7F;
            glyph[3]=0x01; glyph[4]=0x01;
            break;

        case 'U':
            glyph[0]=0x3F; glyph[1]=0x40; glyph[2]=0x40;
            glyph[3]=0x40; glyph[4]=0x3F;
            break;

        case 'Y':
            glyph[0]=0x07; glyph[1]=0x08; glyph[2]=0x70;
            glyph[3]=0x08; glyph[4]=0x07;
            break;

        case ':':
            glyph[1]=0x36;
            glyph[3]=0x36;
            break;

        case '%':
            glyph[0]=0x63;
            glyph[1]=0x13;
            glyph[2]=0x08;
            glyph[3]=0x64;
            glyph[4]=0x63;
            break;

        case '-':
            glyph[0]=0x08;
            glyph[1]=0x08;
            glyph[2]=0x08;
            glyph[3]=0x08;
            glyph[4]=0x08;
            break;

        case '/':
            glyph[0]=0x20;
            glyph[1]=0x10;
            glyph[2]=0x08;
            glyph[3]=0x04;
            glyph[4]=0x02;
            break;

        case ' ':
        default:
            break;
    }
}


/* ---------------- DRAW CHARACTER ---------------- */

void ILI9341_DrawChar(uint16_t x, uint16_t y,
                      char c,
                      uint16_t color,
                      uint16_t bg,
                      uint8_t size)
{
    uint8_t glyph[5];

    GetGlyph(c, glyph);

    for (uint8_t col = 0; col < 5; col++)
    {
        for (uint8_t row = 0; row < 7; row++)
        {
            uint16_t pixelColor;

            if (glyph[col] & (1 << row))
                pixelColor = color;
            else
                pixelColor = bg;

            for (uint8_t dx = 0; dx < size; dx++)
            {
                for (uint8_t dy = 0; dy < size; dy++)
                {
                    ILI9341_DrawPixel(
                        x + col * size + dx,
                        y + row * size + dy,
                        pixelColor
                    );
                }
            }
        }
    }
}


/* ---------------- DRAW STRING ---------------- */

void ILI9341_DrawString(uint16_t x, uint16_t y,
                        const char *str,
                        uint16_t color,
                        uint16_t bg,
                        uint8_t size)
{
    while (*str)
    {
        ILI9341_DrawChar(
            x, y, *str,
            color, bg, size
        );

        x += 6 * size;
        str++;
    }
}
