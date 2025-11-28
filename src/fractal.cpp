#include "fractal.h"
#include "main.h"

// Animation state
static float realDelta = 0.01f;
static float imagDelta = 0.02f;

// Color palette for fractal rendering
static RGBColor colors[16];

void initFractal() {
    // Initialize 16-color EGA-style palette
    colors[0] = RGBColor(0, 0, 0);           // BLACK
    colors[1] = RGBColor(0, 0, 170);         // BLUE
    colors[2] = RGBColor(0, 170, 0);         // GREEN
    colors[3] = RGBColor(0, 170, 170);       // CYAN
    colors[4] = RGBColor(170, 0, 0);         // RED
    colors[5] = RGBColor(170, 0, 170);       // MAGENTA
    colors[6] = RGBColor(170, 85, 0);        // BROWN
    colors[7] = RGBColor(170, 170, 170);     // LIGHTGRAY
    colors[8] = RGBColor(85, 85, 85);        // DARKGRAY
    colors[9] = RGBColor(85, 85, 255);       // LIGHTBLUE
    colors[10] = RGBColor(85, 255, 85);      // LIGHTGREEN
    colors[11] = RGBColor(85, 255, 255);     // LIGHTCYAN
    colors[12] = RGBColor(255, 85, 85);      // LIGHTRED
    colors[13] = RGBColor(255, 85, 255);     // LIGHTMAGENTA
    colors[14] = RGBColor(255, 255, 85);     // YELLOW
    colors[15] = RGBColor(255, 255, 255);    // WHITE
}

void displayFractal() {
    // Recreate sprite if it was deleted
    if (!sprite.created()) {
        sprite.createSprite(320, 240);
        sprite.setPivot(160, 120);
        sprite.setColorDepth(16);
        sprite.setSwapBytes(true);
    }
    // HBRUSH brush is now our sprite's buffer
    int loopCount = 0;
    
    // Julia set constant - different values create different Julia sets
    // Try these interesting values:
    // c = -0.7 + 0.27015i (classic)
    // c = 0.285 + 0.01i (nice spiral)
    // c = -0.4 + 0.6i (dendrite)
    // c = -0.8 + 0.156i (rabbit)

    // Classic Julia set
    // float c_real = -0.7f;
    // float c_imag = 0.27015f;

    float c_real = 0.285f;
    float c_imag = 0.01f;

    // View area in complex plane
    float left = -1.5f;
    float top = -1.0f;
    float xside = 3.0f;  // Width: from -1.5 to 1.5
    float yside = 2.0f;  // Height: from -1.0 to 1.0

    float xscale, yscale, zx, zy, tempx;
    int x, y, count;
    // Get width/height from sprite
    int maxx = sprite.width();
    int maxy = sprite.height();

    // Scale factors to map pixel coordinates to complex plane
    xscale = xside / maxx;
    yscale = yside / maxy;

    // Update Julia constant over time for animation
    realDelta += 0.01f;
    imagDelta += 0.01f;
    c_real += realDelta;
    c_imag += imagDelta;

    float limits = 0.2f;

    if (realDelta > limits || realDelta < -limits) {
        realDelta = -realDelta;
    }

    if (imagDelta > limits || imagDelta < -limits) {
        imagDelta = -imagDelta;
    }

    // Scan every pixel
    for (y = 0; y < maxy; y++) {
        // Check for button press at the start of each row for responsiveness
        // Checking every row (240 times) is much more efficient than every pixel (76,800 times)
        if (digitalRead(PIN_BTN1) == LOW) {
            // Button pressed during rendering - abort and open menu
            btn1Pressed();
            return; // Exit fractal rendering
        }
        
        for (x = 0; x < maxx; x++)
        {
            // Starting z value = pixel position in complex plane
            zx = x * xscale + left;
            zy = y * yscale + top;
            
            count = 0;

            // Iterate: z = z*z + c (where c is constant)
            while ((zx * zx + zy * zy < 4) && (count < MAXCOUNT))
            {
                tempx = zx * zx - zy * zy + c_real;
                zy = 2 * zx * zy + c_imag;
                zx = tempx;
                count++;
            }

            // Color based on iteration count
            if (count == MAXCOUNT) {
                sprite.drawPixel(x, y, TFT_BLACK);
                //SetPixel(m_memDC, x, y, RGB(0, 0, 0));
            } else {
                // SetPixel(m_memDC, x, y, colors[count % 16].ToColorRef());
                sprite.drawPixel(x, y, colors[count % 16].ToColorRef());
            }
        }
    }
    //std::cout << "Julia set c = " << c_real << " + " << c_imag << "i" << std::endl;
    // Push sprite to display without rotation
    //sprite.pushRotated(0);
    sprite.pushRotated(90);
    
    // Note: This is computationally intensive - each frame calculates
    // up to 320x240x30 = 2,304,000 iterations. Consider reducing resolution
    // or MAXCOUNT if performance is an issue.
}


