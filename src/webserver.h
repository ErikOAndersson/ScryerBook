#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <WiFi.h>

// Function declarations
void handleWebServer();
String urlDecode(String str);
unsigned char h2int(char c);

// External references to variables defined in main.cpp
extern WiFiServer webServer;
extern String wifiSSID;
extern String wifiPassword;
extern String imageServerAddress;
extern String imageServerPath;
extern unsigned long fetchInterval;
extern bool useStaticImage;
extern bool useClockMode;

// External function from main.cpp
extern void saveSettings();

#endif
