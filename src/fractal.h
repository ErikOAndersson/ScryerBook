#ifndef FRACTAL_H
#define FRACTAL_H

#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip

#define MAXCOUNT 30

extern TFT_eSprite sprite; // Create a sprite instance

struct RGBColor
{
    unsigned char r;
    unsigned char g;
    unsigned char b;

    RGBColor(unsigned char red = 0, unsigned char green = 0, unsigned char blue = 0)
        : r(red), g(green), b(blue) {}
    
    // Convert to RGB565 format (5 bits red, 6 bits green, 5 bits blue)
    uint16_t ToColorRef() const
    {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
};

void displayFractal();
void initFractal();

#endif // FRACTAL_H

