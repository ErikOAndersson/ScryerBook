#ifndef CLOCK_H
#define CLOCK_H

#include <TFT_eSPI.h>
#include <time.h>

// Clock configuration
#define CLOCK_WIDTH 200
#define CLOCK_HEIGHT 200
#define CLOCK_SCALE 0.8f

// Time information
extern struct tm timeinfo;

// Clock functions
void initClock();
void DrawClockFace();
void DrawClockFace(int xpos, int ypos, float scale);
void UpdateClockFace();

#endif // CLOCK_H
