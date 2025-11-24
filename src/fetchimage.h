#ifndef FETCHIMAGE_H
#define FETCHIMAGE_H

#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include <TFT_eSPI.h>

// Image server configuration - can be changed via web interface
extern String imageServerAddress;
extern int imageServerPort;
extern String imageServerPath;

// WiFi client for HTTPS connections
extern WiFiClientSecure wifi;
extern HttpClient client;

// Image fetching functions
void initFetchImage();
void fetchAndDisplayImage();

// Base64 decoding
size_t base64Decode(const char* input, uint8_t* output, size_t outputSize);

// JPEG rendering
void renderJPEG(int xpos, int ypos);
void drawJpegFromBase64(const char* base64String, int xpos, int ypos);

#endif // FETCHIMAGE_H
