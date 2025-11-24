#include "fetchimage.h"
#include "wifi_credentials.h"
#include "main.h"
#include <JPEGDecoder.h>

// Image server configuration - can be changed via web interface
String imageServerAddress = "witty-cliff-e5fe8cff64ae48afb25807a538c2dad2.azurewebsites.net";
int imageServerPort = 443;  // HTTPS port
String imageServerPath = "/?width=240&height=320&rotate=270";

// WiFi client for HTTPS connections
WiFiClientSecure wifi;
HttpClient client = HttpClient(wifi, imageServerAddress.c_str(), imageServerPort);

// Base64 decoding lookup table
const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Initialize fetch image system
void initFetchImage() {
  // Configure WiFiClientSecure to skip certificate validation (for development)
  // For production, you should use proper certificate validation
  wifi.setInsecure();
  wifi.setTimeout(30000); // Set SSL timeout to 30 seconds (default is 5 seconds)
  
  // Set HttpClient timeout
  client.setTimeout(30000); // Set HTTP client timeout to 30 seconds
}

// Base64 decoder function
size_t base64Decode(const char* input, uint8_t* output, size_t outputSize) {
  size_t inputLen = strlen(input);
  size_t outputLen = 0;
  uint32_t bits = 0;
  int bitCount = 0;
  
  for (size_t i = 0; i < inputLen; i++) {
    char c = input[i];
    
    // Skip whitespace and newlines
    if (c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
    
    // Handle padding
    if (c == '=') break;
    
    // Find character in base64 alphabet
    const char* pos = strchr(base64_chars, c);
    if (!pos) continue;  // Invalid character, skip
    
    int value = pos - base64_chars;
    bits = (bits << 6) | value;
    bitCount += 6;
    
    if (bitCount >= 8) {
      bitCount -= 8;
      if (outputLen < outputSize) {
        output[outputLen++] = (bits >> bitCount) & 0xFF;
      } else {
        return 0; // Output buffer too small
      }
    }
  }
  
  return outputLen;
}

// Render the decoded JPEG to the TFT display (simplified - assumes correct size)
void renderJPEG(int xpos, int ypos) {
  extern TFT_eSPI tft;
  
  Serial.print("JPEG dimensions: ");
  Serial.print(JpegDec.width);
  Serial.print(" x ");
  Serial.println(JpegDec.height);
  
  // Read and display each MCU block
  // Use readSwappedBytes() for proper RGB565 format with byte-swapping
  while (JpegDec.readSwappedBytes()) {
    int mcu_x = JpegDec.MCUx * JpegDec.MCUWidth + xpos;
    int mcu_y = JpegDec.MCUy * JpegDec.MCUHeight + ypos;
    
    // Don't rotate for now, just push normally
    tft.pushImage(mcu_x, mcu_y, JpegDec.MCUWidth, JpegDec.MCUHeight, JpegDec.pImage);
  }
}

// Decode Base64 JPEG and display it
void drawJpegFromBase64(const char* base64String, int xpos, int ypos) {
  // Just return if string is empty
  if (base64String == NULL || strlen(base64String) == 0) {
    Serial.println("ERROR: Empty Base64 string");
    return;
  }

  size_t base64Len = strlen(base64String);
  size_t maxDecodedSize = (base64Len * 3) / 4 + 1;
  
  Serial.print("Base64 string length: "); Serial.println(base64Len);
  Serial.print("Estimated decoded size: "); Serial.println(maxDecodedSize);
  
  // Allocate buffer for decoded JPEG data
  uint8_t* jpegData = (uint8_t*)malloc(maxDecodedSize);
  if (!jpegData) {
    Serial.println("ERROR: Failed to allocate memory for JPEG data");
    return;
  }

  Serial.println("First 30 characters of B64 string:");
  for (int i = 0; i < 30 && i < base64Len; i++) {
    Serial.print(base64String[i]);
  }
  Serial.println();
  
  // Decode Base64 to binary JPEG
  size_t jpegSize = base64Decode(base64String, jpegData, maxDecodedSize);
  
  if (jpegSize == 0) {
    Serial.println("ERROR: Base64 decode failed");
    free(jpegData);
    return;
  }
  
  Serial.print("Decoded JPEG size: "); Serial.println(jpegSize);
  
  // Decode JPEG from memory buffer
  bool decoded = JpegDec.decodeArray(jpegData, jpegSize);
  
  if (!decoded) {
    Serial.println("ERROR: JPEG decode failed");
    free(jpegData);
    return;
  }
  
  // Render the JPEG to display
  renderJPEG(xpos, ypos);
  
  // Clean up
  free(jpegData);
  Serial.println("JPEG displayed successfully");
}

// Fetch image from server and display it
void fetchAndDisplayImage() {
  extern TFT_eSprite sprite;
  extern TFT_eSPI tft;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    // Don't delete sprite if WiFi not connected
    return;
  }
  
  Serial.println("Fetching image from server...");
  
  // Delete sprite to free ~150KB of RAM for image buffers
  sprite.deleteSprite();
  Serial.println("Sprite deleted to free memory");
  
  Serial.print("Connecting to: ");
  Serial.print(imageServerAddress);
  Serial.print(":");
  Serial.println(imageServerPort);
  
  // Ensure certificate validation is disabled (for self-signed certs)
  wifi.setInsecure();
  
  // Stop any existing connection
  wifi.stop();
  
  // Connect directly using WiFiClientSecure
  Serial.println("Attempting SSL connection...");
  if (!wifi.connect(imageServerAddress.c_str(), imageServerPort)) {
    Serial.println("Connection failed!");
    // Recreate sprite before returning
    sprite.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    sprite.setPivot(SPRITE_WIDTH/2, SPRITE_HEIGHT/2);
    return;
  }
  
  Serial.println("Connected! Sending HTTP request...");
  
  // Send HTTP GET request manually
  wifi.print("GET ");
  wifi.print(imageServerPath);
  wifi.println(" HTTP/1.1");
  wifi.print("Host: ");
  wifi.println(imageServerAddress);
  wifi.println("Connection: close");
  wifi.println();
  
  Serial.println("Waiting for response...");
  
  // Wait for response with timeout
  unsigned long timeout = millis();
  while (!wifi.available() && (millis() - timeout < 10000)) {
    delay(10);
  }
  
  if (!wifi.available()) {
    Serial.println("Response timeout!");
    wifi.stop();
    // Recreate sprite before returning
    sprite.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    sprite.setPivot(SPRITE_WIDTH/2, SPRITE_HEIGHT/2);
    return;
  }
  
  // Read status line
  String statusLine = wifi.readStringUntil('\n');
  Serial.print("Status: ");
  Serial.println(statusLine);
  
  // Skip headers - read and display them for debugging
  Serial.println("Headers:");
  int headerCount = 0;
  while (wifi.available()) {
    String line = wifi.readStringUntil('\n');
    line.trim(); // Remove \r and whitespace
    
    if (line.length() == 0) {
      Serial.println("--- End of headers ---");
      break; // Empty line indicates end of headers
    }
    
    Serial.println(line);
    headerCount++;
    
    // Safety check - don't read forever
    if (headerCount > 50) {
      Serial.println("ERROR: Too many headers!");
      break;
    }
  }
  
  // Give a moment for any remaining data to arrive
  delay(100);
  
  // Read response body with chunked encoding support
  Serial.println("Reading response body (chunked encoding)...");
  size_t freeHeap = ESP.getFreeHeap();
  Serial.print("Free heap: ");
  Serial.println(freeHeap);
  
  // Allocate buffer for base64 data - use dynamic size based on available heap
  // Reserve 90KB for system/stack, use rest for buffer (more aggressive to fit larger images)
  size_t maxBase64Size = (freeHeap > 100000) ? (freeHeap - 90000) : 30000;
  
  // Cap at reasonable maximum (increased from 80KB to 100KB)
  if (maxBase64Size > 100000) maxBase64Size = 100000;
  
  Serial.print("Allocating ");
  Serial.print(maxBase64Size);
  Serial.println(" bytes for base64 buffer");
  
  char* base64Buffer = (char*)malloc(maxBase64Size);
  
  if (!base64Buffer) {
    Serial.println("ERROR: Failed to allocate base64 buffer!");
    wifi.stop();
    // Recreate sprite before returning
    sprite.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    sprite.setPivot(SPRITE_WIDTH/2, SPRITE_HEIGHT/2);
    return;
  }
  
  // Read chunked data
  size_t totalBytesRead = 0;
  unsigned long startTime = millis();
  
  while (wifi.connected() && (millis() - startTime < 30000) && totalBytesRead < maxBase64Size - 1) {
    if (wifi.available()) {
      // Read chunk size line (hex number followed by \r\n)
      String chunkSizeLine = wifi.readStringUntil('\n');
      chunkSizeLine.trim();
      
      if (chunkSizeLine.length() == 0) continue; // Skip empty lines
      
      // Parse hex chunk size
      long chunkSize = strtol(chunkSizeLine.c_str(), NULL, 16);
      
      if (chunkSize == 0) {
        Serial.println("Received final chunk (size 0)");
        break; // Last chunk
      }
      
      Serial.print("Reading chunk of size: 0x");
      Serial.print(chunkSize, HEX);
      Serial.print(" (");
      Serial.print(chunkSize);
      Serial.println(" bytes)");
      
      // Read chunk data
      size_t bytesToRead = min((size_t)chunkSize, maxBase64Size - totalBytesRead - 1);
      
      // Warn if we're truncating
      if (bytesToRead < (size_t)chunkSize) {
        Serial.print("WARNING: Buffer full! Truncating chunk from ");
        Serial.print(chunkSize);
        Serial.print(" to ");
        Serial.println(bytesToRead);
      }
      
      size_t chunkBytesRead = 0;
      
      while (chunkBytesRead < bytesToRead && (millis() - startTime < 30000)) {
        if (wifi.available()) {
          int bytesRead = wifi.read((uint8_t*)(base64Buffer + totalBytesRead + chunkBytesRead), 
                                    bytesToRead - chunkBytesRead);
          if (bytesRead > 0) {
            chunkBytesRead += bytesRead;
          }
        } else {
          delay(5);
        }
      }
      
      totalBytesRead += chunkBytesRead;
      
      // Read trailing \r\n after chunk data
      wifi.readStringUntil('\n');
      
    } else {
      delay(10);
    }
  }
  
  base64Buffer[totalBytesRead] = '\0';
  wifi.stop();
  
  Serial.print("Received ");
  Serial.print(totalBytesRead);
  Serial.println(" base64 characters");
  Serial.print("Free heap after read: ");
  Serial.println(ESP.getFreeHeap());
  
  if (totalBytesRead > 0) {
    // Show first and last few characters for debugging
    Serial.print("First 20 chars: ");
    for (int i = 0; i < 20 && i < totalBytesRead; i++) {
      Serial.print(base64Buffer[i]);
    }
    Serial.println();
    Serial.print("Last 20 chars: ");
    for (int i = max(0, (int)totalBytesRead - 20); i < totalBytesRead; i++) {
      Serial.print(base64Buffer[i]);
    }
    Serial.println();
    
    // Decode base64 to binary JPEG
    size_t maxJpegSize = (totalBytesRead * 3) / 4 + 1;
    uint8_t* jpegData = (uint8_t*)malloc(maxJpegSize);
    
    if (!jpegData) {
      Serial.println("ERROR: Failed to allocate JPEG buffer!");
      free(base64Buffer);
      // Recreate sprite before returning
      sprite.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
      sprite.setPivot(SPRITE_WIDTH/2, SPRITE_HEIGHT/2);
      return;
    }
    
    size_t jpegSize = base64Decode(base64Buffer, jpegData, maxJpegSize);
    free(base64Buffer);  // Free base64 buffer immediately after decode
    
    Serial.print("Decoded to ");
    Serial.print(jpegSize);
    Serial.println(" JPEG bytes");
    Serial.print("Free heap after decode: ");
    Serial.println(ESP.getFreeHeap());
    
    // Verify JPEG header (should start with FF D8 FF)
    if (jpegSize > 3) {
      Serial.print("JPEG header: ");
      Serial.print(jpegData[0], HEX);
      Serial.print(" ");
      Serial.print(jpegData[1], HEX);
      Serial.print(" ");
      Serial.println(jpegData[2], HEX);
      
      if (jpegData[0] != 0xFF || jpegData[1] != 0xD8) {
        Serial.println("ERROR: Invalid JPEG header! Data may be corrupted.");
      }
    }
    
    if (jpegSize > 0) {
      // Decode and render JPEG from memory
      tft.fillScreen(TFT_BLACK);
      
      bool decoded = JpegDec.decodeArray(jpegData, jpegSize);
      if (decoded) {
        renderJPEG(0, 0);
        Serial.println("JPEG rendered successfully!");
      } else {
        Serial.println("ERROR: JPEG decode failed!");
      }
    }
    
    free(jpegData);
  } else {
    free(base64Buffer);
    Serial.println("ERROR: Empty response!");
  }
  
  // Recreate sprite for other modes (clock, quote, etc.)
  sprite.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
  sprite.setPivot(SPRITE_WIDTH/2, SPRITE_HEIGHT/2);
  Serial.println("Sprite recreated");
}
