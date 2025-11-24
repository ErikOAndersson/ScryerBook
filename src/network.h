#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <IPAddress.h>

// Network Profile Structure
struct NetworkProfile {
  const char* name;
  IPAddress staticIP;
  IPAddress gateway;
  IPAddress subnet;
  IPAddress primaryDNS;
  IPAddress secondaryDNS;
};

// Network management functions
void initNetwork();
void connectWiFi();
bool tryConnectWiFi();
void loadNetworkProfile(int profileIndex);

// Network configuration access
extern NetworkProfile networkProfiles[];
extern const int numNetworkProfiles;
extern int currentProfileIndex;

// Active network configuration
extern IPAddress staticIP;
extern IPAddress gateway;
extern IPAddress subnet;
extern IPAddress primaryDNS;
extern IPAddress secondaryDNS;

// WiFi credentials (defined in wifi_credentials.h)
extern String wifiSSID;
extern String wifiPassword;

#endif // NETWORK_H
