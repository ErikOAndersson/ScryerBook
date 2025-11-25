#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

//#define WIFI_SSID "SeriesOfTubes4"
//#define WIFI_PASSWORD "gnugnu4096"

//#define WIFI_SSID "ErikCell"
//#define WIFI_PASSWORD "gnugnu64"

#define WIFI_SSID "SignUp Guest"
#define WIFI_PASSWORD "Dynamics"

// API Ninjas API key for quote service
//#define API_NINJAS_KEY "YOUR_API_KEY_HERE"

// Network Profiles Configuration
#define NETWORK_PROFILES \
  { \
    "Home", \
    IPAddress(192, 168, 1, 61), \
    IPAddress(192, 168, 1, 1), \
    IPAddress(255, 255, 255, 0), \
    IPAddress(8, 8, 8, 8), \
    IPAddress(8, 8, 4, 4), \
    "SeriesOfTubes4", \
    "gnugnu4096" \
  }, \
  { \
    "Work", \
    IPAddress(172, 31, 0, 250), \
    IPAddress(172, 31, 0, 1), \
    IPAddress(255, 255, 254, 0), \
    IPAddress(1, 1, 1, 1), \
    IPAddress(9, 9, 9, 9), \
    "SignUp Guest", \
    "Dynamics" \
  }, \
  { \
    "MobileWork", \
    IPAddress(10, 10, 200, 251), \
    IPAddress(10, 10, 200, 1), \
    IPAddress(255, 255, 255, 0), \
    IPAddress(10, 10, 3, 100), \
    IPAddress(10, 10, 3, 101), \
    "ErikCell", \
    "gnugnu64" \
  }

#endif
