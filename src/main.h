#include <Arduino.h>

#ifndef __MAIN_H__
#define __MAIN_H__ 

#define ENABLE_WIFI true
// Full screen sprite dimensions (rotated - we rotate the sprite 90° when displaying)
#define SPRITE_WIDTH 320
#define SPRITE_HEIGHT 240

enum MODE { FETCH, CLOCK, ALBUM, QUOTE, STATUSINFO, NONE  };
#define VERSION "1.0.1115"

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
void displayStatusInfo();
void btn1Pressed();
bool tryConnectWiFi();

#endif // __MAIN_H__


