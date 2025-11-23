#include <Arduino.h>

#ifndef __MAIN_H__
#define __MAIN_H__ 

#define CLOCK_WIDTH 200
#define CLOCK_HEIGHT 200
#define CLOCK_SCALE 0.8f

#define ENABLE_WIFI true
// Full screen sprite dimensions (rotated - we rotate the sprite 90° when displaying)
#define SPRITE_WIDTH 320
#define SPRITE_HEIGHT 240

// Api from api-ninjas.com
#define API_NINJAS_URL "https://api.api-ninjas.com/v1/"
#define API_NINJAS_KEY "oc9IJzYB5MIJToPwxULcEg==7Xsm6HptY9Pclqk8"

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
void UpdateClockFace();
void DrawClockFace();
void DrawClockFace(int xpos, int ypos, float scale);
char* GetNinjaQuote();  // Returns malloc'd memory - CALLER MUST FREE!
void displayStatusInfo();
void btn1Pressed();

#endif // __MAIN_H__


