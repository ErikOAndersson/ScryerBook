#include <Arduino.h>

#ifndef __MAIN_H__
#define __MAIN_H__ 

#define CLOCK_WIDTH 200
#define CLOCK_HEIGHT 200
#define CLOCK_SCALE 0.8f

// Full screen sprite dimensions (rotated - we rotate the sprite 90° when displaying)
#define SPRITE_WIDTH 320
#define SPRITE_HEIGHT 240

// Function declarations5
void Blip(int blips);
void drawJpegFromBase64(const char* base64String, int xpos, int ypos);
void drawRGB565Image(const uint16_t* imageData, int width, int height, int xpos, int ypos);
void renderJPEG(int xpos, int ypos);
size_t base64Decode(const char* input, uint8_t* output, size_t outputSize);
void connectWiFi();
void fetchAndDisplayImage();
void loadSettings();
void saveSettings();
void handleWebServer();
String urlDecode(String str);
unsigned char h2int(char c);
void UpdateClockFace();
void DrawClockFace();
void DrawClockFace(int xpos, int ypos, float scale);







#endif // __MAIN_H__


