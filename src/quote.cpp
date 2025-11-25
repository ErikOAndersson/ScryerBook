#include "quote.h"
#include "wifi_credentials.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// WiFi client for HTTPS connections (persistent instance)
WiFiClientSecure quoteClient;

// Initialize quote system
void initQuote() {
  // Configure WiFiClientSecure to skip certificate validation
  quoteClient.setInsecure();
  quoteClient.setTimeout(10000); // Set timeout to 10 seconds
}

// Function GetNinjaQuote(), return malloc'd char array (CALLER MUST FREE!)
// Returns nullptr on error
char* GetNinjaQuote() {
  const char* host = API_NINJAS_HOST;
  const int httpsPort = 443;
  
  Serial.print("Connecting to API Ninjas (");
  Serial.print(host);
  Serial.println(")...");
  
  // Check DNS resolution first
  IPAddress resolvedIP;
  if (WiFi.hostByName(host, resolvedIP)) {
    Serial.print("DNS resolved to: ");
    Serial.println(resolvedIP);
  } else {
    Serial.println("ERROR: DNS resolution failed for API Ninjas!");
    Serial.print("Current DNS servers: ");
    Serial.print(WiFi.dnsIP(0).toString());
    Serial.print(", ");
    Serial.println(WiFi.dnsIP(1).toString());
    char* result = (char*)malloc(strlen(tmp_quote) + 1);
    if (result) {
      strcpy(result, tmp_quote);
    }
    return result;
  }
  
  // Stop any existing connection
  quoteClient.stop();
  
  if (!quoteClient.connect(host, httpsPort)) {
    Serial.println("ERROR: Failed to connect to quote API. Using temporary quote.");
    char* result = (char*)malloc(strlen(tmp_quote) + 1);
    if (result) {
      strcpy(result, tmp_quote);
    }
    return result;
  }
  
  // Send HTTP GET request
  quoteClient.print("GET ");
  quoteClient.print(API_NINJAS_PATH);
  quoteClient.print(" HTTP/1.1\r\n");
  quoteClient.print("Host: ");
  quoteClient.print(host);
  quoteClient.print("\r\n");
  quoteClient.print("X-Api-Key: ");
  quoteClient.print(API_NINJAS_KEY);
  quoteClient.print("\r\n");
  quoteClient.print("Connection: close\r\n");
  quoteClient.print("\r\n");
  
  // Wait for response
  unsigned long timeout = millis();
  while (quoteClient.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("ERROR: API request timeout");
      quoteClient.stop();
      return nullptr;
    }
  }
  
  // Read status line
  String statusLine = quoteClient.readStringUntil('\n');
  Serial.print("API Status: ");
  Serial.println(statusLine);
  
  // Skip headers
  while (quoteClient.available()) {
    String line = quoteClient.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      break; // End of headers
    }
  }
  
  // Read response body (simple read, API response should be small)
  String response = "";
  while (quoteClient.available()) {
    response += (char)quoteClient.read();
  }
  
  quoteClient.stop();
  
  Serial.println("Response: " + response);
  
  // Parse JSON and extract quote
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);
  if (!error) {
    const char* quote = doc[0]["quote"]; // API returns array, get first element
    if (quote) {
      // Allocate memory and copy the quote string
      size_t len = strlen(quote);
      char* result = (char*)malloc(len + 1);
      if (result) {
        strcpy(result, quote);
        return result;
      } else {
        Serial.println("ERROR: malloc failed");
        return nullptr;
      }
    }
  }
  
  Serial.print("ERROR: JSON parsing failed: ");
  Serial.println(error.c_str());
  return nullptr;
}
