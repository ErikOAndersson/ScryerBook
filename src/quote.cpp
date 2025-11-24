#include "quote.h"
#include "wifi_credentials.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Initialize quote system
void initQuote() {
  // Nothing to initialize currently
}

// Function GetNinjaQuote(), return malloc'd char array (CALLER MUST FREE!)
// Returns nullptr on error
char* GetNinjaQuote() {
  // Setup GET request to quote API using WiFiClientSecure
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation
  
  const char* host = API_NINJAS_URL;    // "api.api-ninjas.com";
  const int httpsPort = 443;
  
  Serial.println("Connecting to API Ninjas...");
  if (!client.connect(host, httpsPort)) {
    Serial.println("ERROR: Failed to connect to quote API. Using temporary quote.");
    char* result = (char*)malloc(strlen(tmp_quote) + 1);
    if (result) {
      strcpy(result, tmp_quote);
    }
    return result;
  }
  
  // Send HTTP GET request
  client.print("GET /v1/quotes HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\n");
  client.print("X-Api-Key: ");
  client.print(API_NINJAS_KEY);
  client.print("\r\n");
  client.print("Connection: close\r\n");
  client.print("\r\n");
  
  // Wait for response
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("ERROR: API request timeout");
      client.stop();
      return nullptr;
    }
  }
  
  // Read status line
  String statusLine = client.readStringUntil('\n');
  Serial.print("API Status: ");
  Serial.println(statusLine);
  
  // Skip headers
  while (client.available()) {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
      break; // End of headers
    }
  }
  
  // Read response body (simple read, API response should be small)
  String response = "";
  while (client.available()) {
    response += (char)client.read();
  }
  
  client.stop();
  
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
