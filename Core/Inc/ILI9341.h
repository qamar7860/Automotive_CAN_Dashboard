#ifndef INC_ILI9341_H_
#define INC_ILI9341_H_

#include "main.h"

#define TFT_WIDTH   240
#define TFT_HEIGHT  320

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define ORANGE  0xFD20
#define GRAY    0x8410

void ILI9341_Init(void);
void ILI9341_FillScreen(uint16_t color);

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

void ILI9341_FillRect(uint16_t x, uint16_t y,
                      uint16_t w, uint16_t h,
                      uint16_t color);

void ILI9341_DrawChar(uint16_t x, uint16_t y,
                      char c,
                      uint16_t color,
                      uint16_t bg,
                      uint8_t size);

void ILI9341_DrawString(uint16_t x, uint16_t y,
                        const char *str,
                        uint16_t color,
                        uint16_t bg,
                        uint8_t size);

#endif
