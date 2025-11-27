#include <Arduino.h>
#include <TFT_eSPI.h>

#ifndef __MAIN_H__
#define __MAIN_H__ 

#define ENABLE_WIFI true
// Full screen sprite dimensions (rotated - we rotate the sprite 90° when displaying)
#define SPRITE_WIDTH 320
#define SPRITE_HEIGHT 240

enum MODE { FETCH, CLOCK, ALBUM, QUOTE, STATUSINFO, NETWORK, SCREENSAVER, NONE  };
enum ALBUM_MODE_TYPE { NATURE, SYNTHWAVE };
#define VERSION "1.0.1126"

extern ALBUM_MODE_TYPE albumMode;

// Function declarations5
void Blip(int blips);
void drawJpegFromBase64(const char* base64String, int xpos, int ypos);
void drawRGB565Image(const uint16_t* imageData, int width, int height, int xpos, int ypos);
void renderJPEG(int xpos, int ypos);
size_t base64Decode(const char* input, uint8_t* output, size_t outputSize);
bool saveJpegToLittleFS(const char* filename, const uint8_t* jpegData, size_t jpegSize);
bool loadJpegFromLittleFS(const char* filename, int xpos, int ypos);
bool loadJpegToSpriteFromLittleFS(TFT_eSprite &spr, const char* filename, int xpos, int ypos);
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
void displayNetworkPicker();
void connectToWiFiNetwork(int currentProfileIndex);
#endif // __MAIN_H__


