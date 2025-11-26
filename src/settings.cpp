#include "settings.h"
#include "main.h"
#include <Preferences.h>
#include "wifi_credentials.h"

// External references to global variables from main.cpp
extern String wifiSSID;
extern String wifiPassword;
extern String imageServerAddress;
extern String imageServerPath;
extern unsigned long fetchInterval;
extern bool useStaticImage;
extern bool useClockMode;
extern Preferences preferences;
extern MODE _mode;
extern ALBUM_MODE_TYPE albumMode;

// Load settings from NVS preferences
void loadSettings() {
  preferences.begin("scryerbook", false);
  
  // Load WiFi settings
  // REMOVE THIS (comment) hubba
  //wifiSSID = preferences.getString("wifiSSID", WIFI_SSID);
  //wifiPassword = preferences.getString("wifiPassword", WIFI_PASSWORD);
 
  
  // Load image server settings
  // REMOVE THIS (comment) hubba
  //imageServerAddress = preferences.getString("imageServer", "witty-cliff-e5fe8cff64ae48afb25807a538c2dad2.azurewebsites.net");
  //imageServerPath = preferences.getString("imagePath", "/?width=240&height=320&rotate=90");
  
  // Load timing settings
  fetchInterval = preferences.getULong("fetchInterval", 20000);
  
  // Load static image setting
  useStaticImage = preferences.getBool("useStatic", false);
  useStaticImage = true; // Force static image mode for now, remember to change later!

  // Load clock mode setting
  useClockMode = preferences.getBool("useClockMode", false);

  // Load display mode (default to CLOCK)
  _mode = (MODE)preferences.getUChar("displayMode", CLOCK);
  
  // Load album mode (default to NATURE)
  albumMode = (ALBUM_MODE_TYPE)preferences.getUChar("albumMode", NATURE);

  preferences.end();
  
  Serial.println("Settings loaded from preferences");
}

// Save settings to NVS preferences
void saveSettings() {
  preferences.begin("scryerbook", false);
  
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPassword", wifiPassword);
  preferences.putString("imageServer", imageServerAddress);
  preferences.putString("imagePath", imageServerPath);
  preferences.putULong("fetchInterval", fetchInterval);
  preferences.putBool("useStatic", useStaticImage);
  preferences.putBool("useClockMode", useClockMode);
  preferences.putUChar("displayMode", (uint8_t)_mode);
  preferences.putUChar("albumMode", (uint8_t)albumMode);
  
  preferences.end();
  
  Serial.println("Settings saved to preferences");
}
