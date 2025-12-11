#include "network.h"
#include "wifi_credentials.h"

// ======================================
// NETWORK PROFILE SELECTION
// Set this to "Home" or "Work" to force a specific profile
// ======================================
//const char* PREFERRED_NETWORK = "Work";  // For testing, this should be saved in settings otherwise
const char* PREFERRED_NETWORK = "MobileWork";  // For testing, this should be saved in settings otherwise

// WiFi credentials - can be overridden by web settings
String wifiSSID = WIFI_SSID;
String wifiPassword = WIFI_PASSWORD;

// Network Profiles - Add more as needed
// Defined in wifi_credentials.h to keep sensitive data out of repository
NetworkProfile networkProfiles[] = {
  NETWORK_PROFILES
};

const int numNetworkProfiles = sizeof(networkProfiles) / sizeof(networkProfiles[0]);
//int currentProfileIndex = 0;  // Start with first profile (Home)
int currentProfileIndex = 2;  // Start with profile index 2 (MobileWork) for testing

// Active network configuration (loaded from current profile)
IPAddress staticIP;
IPAddress gateway;
IPAddress subnet;
IPAddress primaryDNS;
IPAddress secondaryDNS;

// Initialize network system
void initNetwork() {
  // Find and load the preferred network profile
  for (int i = 0; i < numNetworkProfiles; i++) {
    if (strcmp(networkProfiles[i].name, PREFERRED_NETWORK) == 0) {
      currentProfileIndex = i;
      Serial.print("Using preferred network profile: ");
      Serial.println(PREFERRED_NETWORK);
      break;
    }
  }
  
  // Load the selected profile
  loadNetworkProfile(currentProfileIndex);
}

// Load network profile settings into active configuration
void loadNetworkProfile(int profileIndex) {
  if (profileIndex < 0 || profileIndex >= numNetworkProfiles) {
    Serial.println("Invalid profile index!");
    return;
  }
  
  NetworkProfile& profile = networkProfiles[profileIndex];
  
  staticIP = profile.staticIP;
  gateway = profile.gateway;
  subnet = profile.subnet;
  primaryDNS = profile.primaryDNS;
  secondaryDNS = profile.secondaryDNS;
  
  Serial.print("Loaded network profile: ");
  Serial.println(profile.name);
  Serial.print("  IP: ");
  Serial.println(staticIP);
  Serial.print("  Gateway: ");
  Serial.println(gateway);
  Serial.print("  DNS: ");
  Serial.print(primaryDNS);
  Serial.print(", ");
  Serial.println(secondaryDNS);
}

// Connect to WiFi with automatic profile fallback
void connectWiFi() {
  // Try current profile first
  loadNetworkProfile(currentProfileIndex);
  
  bool connected = tryConnectWiFi();
  
  // If failed, try other profiles
  if (!connected) {
    Serial.println("Connection failed with current profile. Trying other profiles...");
    
    for (int i = 0; i < numNetworkProfiles; i++) {
      if (i == currentProfileIndex) continue; // Skip already tried profile
      
      Serial.print("Trying profile ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(networkProfiles[i].name);
      
      loadNetworkProfile(i);
      connected = tryConnectWiFi();
      
      if (connected) {
        currentProfileIndex = i; // Remember successful profile
        Serial.print("Successfully connected using profile: ");
        Serial.println(networkProfiles[i].name);
        break;
      }
    }
  }
  
  if (!connected) {
    Serial.println("Failed to connect with any network profile!");
  }
}

// Attempt to connect to WiFi with current settings
bool tryConnectWiFi() {
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(wifiSSID);
  
  // Stop any existing connection
  WiFi.disconnect(true);
  delay(100);
  
  // Configure static IP with DNS servers
  if (!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("WiFi.config failed!");
    return false;
  }
  
  // Begin WiFi connection
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  // Wait briefly for connection to establish
  delay(100);
  
  // Reapply DNS configuration (some networks override DNS settings)
  if (!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Failed to reapply DNS configuration");
  }
  
  // Wait for connection (10 seconds max)
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("DNS Server 1: ");
    Serial.println(WiFi.dnsIP(0));
    Serial.print("DNS Server 2: ");
    Serial.println(WiFi.dnsIP(1));
    return true;
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
    return false;
  }
}
