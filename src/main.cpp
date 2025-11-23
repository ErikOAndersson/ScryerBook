#include "main.h"
#include "webserver.h"
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "LittleFS.h"
//#include "beach.h"
#include "wifi_credentials.h"
#include <math.h>
//#include "face1.h"
//#include "static1.h"
#include "font_gilligan_10.h"
#include "time.h"

#define min(a,b)     (((a) < (b)) ? (a) : (b))

// ToDo: Move this to main.h later
void loadImageToSpriteFromLittleFS(TFT_eSprite &spr, const char* filename, int xpos, int ypos, int imageWidth = 320, int imageHeight = 0, uint16_t transparentColor = 0xFFFF);
void drawWrappedText(TFT_eSprite &spr, const char* text, int x, int y, int maxWidth, int lineHeight);


// Preferences for storing settings
Preferences preferences;

// WiFi credentials - can be overridden by web settings
String wifiSSID = WIFI_SSID;
String wifiPassword = WIFI_PASSWORD;

// Static IP configuration
IPAddress staticIP(192, 168, 1, 61);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

WiFiClientSecure wifi;
WiFiServer webServer(80);  // Web server on port 80

// Timey-wimey stuff
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 7200;


// Image server configuration - can be changed via web interface
String imageServerAddress = "witty-cliff-e5fe8cff64ae48afb25807a538c2dad2.azurewebsites.net";
int imageServerPort = 443;  // HTTPS port
String imageServerPath = "/?width=240&height=320&rotate=90";

// Local testing server
// String imageServerAddress = "192.168.1.55";1
// int imageServerPort = 8443;  // HTTP port
// String imageServerPath = "/index.html";

bool useStaticImage = false;
bool useClockMode = false;

// Create HTTPS client
HttpClient client = HttpClient(wifi, imageServerAddress.c_str(), imageServerPort);

// Timing - can be changed via web interface
unsigned long lastFetchTime = 99999999; // Force immediate fetch on startup
unsigned long fetchInterval = 20000; // 20 seconds (default)
// lastStatusUpdateTime >= statusUpdateInterval
unsigned long lastStatusUpdateTime = 99999999; // Force immediate status update on startup
unsigned long statusUpdateInterval = 5000; // 5 seconds (default)


// TFT instance
TFT_eSPI tft = TFT_eSPI();  

// Sprite stuff for the clock face, and other vars
TFT_eSprite sprite = TFT_eSprite(&tft); // Create a sprite instance
struct tm timeinfo;

// do not forget User_Setup_xxxx.h matching microcontroller-display combination
 // my own: "/include/User_Setup.h"

#define PIN_LED 21
#define PIN_BTN1 8
#define PIN_BTN2 9

int delayTime = 500;
int j,t;
int count = 0;

// Set default startup Mode
// ToDo: Make this a setting
MODE _mode = CLOCK;

// Temporary for testing, remove this
const char tmp_quote[] = "I slept with faith and found a corpse in my arms on awakening I drank and danced all night with doubt and found her a virgin in the morning.";

// ======================================================================
// Setup function
// ======================================================================
void setup() {
  // Initialize pins
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);

  Blip(10);

  Serial.begin (9600);
  delay(1000);  // Give serial time to initialize

  Serial.println("=== ScryerBook Starting ===");

  // Load saved settings from preferences
  Serial.println("Loading settings...");
  loadSettings();

  Serial.println("Initializing TFT...");
  Serial.print("TFT_CS: "); Serial.println(TFT_CS);
  Serial.print("TFT_DC: "); Serial.println(TFT_DC);
  Serial.print("TFT_MOSI: "); Serial.println(TFT_MOSI);
  Serial.print("TFT_SCLK: "); Serial.println(TFT_SCLK);
  Serial.print("TFT_RST: "); Serial.println(TFT_RST);
  
  delay(100);  // Let pins stabilize

  // Disable WiFi for testing
#if ENABLE_WIFI
  // Connect to WiFI with static IP
  connectWiFi();
  
  // Start web server
  webServer.begin();
  Serial.println("Web server started");
  Serial.print("Access settings at: http://");
  Serial.println(WiFi.localIP());

  // Initialize LittleFS AFTER WiFi and TFT
  Serial.println("Mounting LittleFS...");
  if (!LittleFS.begin(false)) {  // false = don't format on failure
    Serial.println("LittleFS Mount Failed!");
  } else {
    Serial.println("LittleFS Mounted Successfully");
    // List files in LittleFS for debugging
    File root = LittleFS.open("/");
    if (root) {
      File file = root.openNextFile();
      Serial.println("Files in LittleFS:");
      while (file) {
        Serial.print("  ");
        Serial.print(file.name());
        Serial.print(" - ");
        Serial.print(file.size());
        Serial.println(" bytes");
        file = root.openNextFile();
      }
    }
  }
  
  // Configure WiFiClientSecure to skip certificate validation (for development)
  // For production, you should use proper certificate validation
  wifi.setInsecure();
  wifi.setTimeout(30000); // Set SSL timeout to 30 seconds (default is 5 seconds)
  
  // Set HttpClient timeout
  client.setTimeout(30000); // Set HTTP client timeout to 30 seconds
#endif // ENABLE_WIFI

  tft.begin();
  tft.setPivot(TFT_WIDTH / 2, TFT_HEIGHT / 2); // Set TFT pivot to center for pushRotated()
  Serial.println("TFT initialized successfully!");
  //tft.fillScreen (BLACK);
  Serial.println ();                                 // cut gibberish in Serial Monitor
  Serial.println ();    
  Serial.println ("Colors array - GC9A01 display");  // test Serial Monitor
  Serial.println ("on ESP32-C3 Super Mini");  
  //Serial.println ();
  // tft.drawCircle (120,120,80,SCOPE);
  tft.fillScreen(TFT_BLACK);

  // Init the full-screen sprite for background + clock
  sprite.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT); // Create full screen sprite (320x240)
  sprite.setPivot(SPRITE_WIDTH/2, SPRITE_HEIGHT/2); // Set pivot point to center of sprite for rotation
  sprite.setColorDepth(16); // Set color depth to 16 bits for better color quality
  sprite.fillSprite(TFT_BLACK); // Fill with black initially
  sprite.setSwapBytes(true); // Ensure correct byte order for pushRotated
  
  // Populate the timeinfo struct with the time 10:15:30 for testing
  // timeinfo.tm_hour = 11;
  // timeinfo.tm_min = 55;
  // timeinfo.tm_sec = 30;

  // Get time from NTP server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  // Wait a moment for NTP to sync
  Serial.println("Waiting for NTP time sync...");
  delay(2000);
  
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time from NTP, using fallback time");
    timeinfo.tm_hour = 11;
    timeinfo.tm_min = 55;
    timeinfo.tm_sec = 30;
  } else {
    Serial.println(&timeinfo, "Current time: %Y-%m-%d %H:%M:%S");
  }

  if (true) {
    // Initialize LittleFS
    Serial.println("Mounting LittleFS...");
    if (!LittleFS.begin()) {
      Serial.println("An error has occurred while mounting LittleFS");
    } else {
      Serial.println("LittleFS mounted successfully");
    }
  }


  
  // Example: Display JPEG from Base64 string
  // Uncomment to test with your Base64-encoded JPEG
  
  //const char* base64Jpeg = "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AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDg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";
  const char* base64Jpeg = "";
  tft.invertDisplay(true); // Invert display colors
  //tft.setSwapBytes(true); // Swap byte order for TFT
  tft.setSwapBytes(false); // This is back again? Trying default.
  tft.fillScreen(TFT_BLACK);

  //http://witty-cliff-e5fe8cff64ae48afb25807a538c2dad2.azurewebsites.net/?width=320&height=240&info&rotate=90

  Serial.println ("Starting up...");

}

// ======================================================================
// Main loop
// ======================================================================
void loop() {
  // Handle web server requests
#if ENABLE_WIFI
  handleWebServer();
#endif

  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  // Testing LED and buttons
/*
  // Print button status to serial
  char b1 = digitalRead(PIN_BTN1) == LOW ? '1' : '0';
  char b2 = digitalRead(PIN_BTN2) == LOW ? '1' : '0';
  Serial.print("Buttons: ");
  Serial.print(b1);
  Serial.print(b2);
  Serial.println();

  if (digitalRead(PIN_BTN1) == LOW) {
    Blip(2);
  }
  if (digitalRead(PIN_BTN2) == LOW) {
    Blip(3);
  }
  delay(100);
  return;
  */
  // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  if (_mode == STATUSINFO) {
    // Status mode - display status info

    unsigned long currentTime = millis();
    if (currentTime - lastStatusUpdateTime >= statusUpdateInterval) {
      lastStatusUpdateTime = currentTime;
      displayStatusInfo();
    }
  }
  if (_mode == QUOTE) {
    // Quote mode - display a new quote every fetchInterval
    unsigned long currentTime = millis();
    
    if (currentTime - lastFetchTime >= fetchInterval) {
      // Get the return result from function GetNinjaQuote() - returns malloc'd memory!
      char *quote = GetNinjaQuote();
      //const char *quote = (char *)  tmp_quote; // Using temporary quote for testing

      if (quote != nullptr) {
        Serial.println("Displaying new quote:");
        Serial.println(quote);
        //drawWrappedText(TFT_eSprite &spr, const char* text, int x, int y, int maxWidth, int lineHeight) {
        // Load background image from LittleFS to sprite
        loadImageToSpriteFromLittleFS(sprite, "/backquote.rgb565", 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT);
        // Clear the display
        tft.fillScreen(TFT_BLACK);
        drawWrappedText(sprite, quote, 20, 30, SPRITE_WIDTH - 40, 30);

        // If this works I'll be surprised
        //loadImageToSpriteFromLittleFS(sprite, "/fleuron.rgb565", 75, 210);
        //loadImageToSpriteFromLittleFS(sprite, "/fleuron.rgb565", 75, 200, 120, 33);
        //loadImageToSpriteFromLittleFS(sprite, "/fleuron.rgb565", 100, 210, 120, 33, 0xF81F);  // 0xF81F = #FF00FF = magenta transparency
        // Load PNG-based image with transparency (magenta = transparent)
        loadImageToSpriteFromLittleFS(sprite, "/fleuron.rgb565", 100, 200, 120, 33, 0xF81F);
        
        sprite.pushRotated(90);

        // // Draw text on the sprite BEFORE rotating
        // sprite.setFreeFont(&GilliganShutter10pt7b);  // Set the custom font on sprite
        // sprite.setTextColor(TFT_WHITE, TFT_BLACK);   // White text, black background
        // sprite.setTextDatum(MC_DATUM);               // Middle center alignment
        // sprite.drawString(quote, SPRITE_WIDTH / 2, SPRITE_HEIGHT / 2);        
        // // Now push the sprite with text rotated 90 degrees
        // sprite.pushRotated(90);
        


        // Free the malloc'd memory if tmp_quote is defined
        // REMOVE THIS comment, we should free  it
        // free(quote);
      } else {
        Serial.println("Failed to get quote.");
      }

      lastFetchTime = currentTime;
    }
  }


  if (_mode == CLOCK) {
    // Call DrawClockFace every second and right at startup
    static unsigned long lastClockFaceTime = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - lastClockFaceTime >= 1000) {
      //DrawClockFace();
      UpdateClockFace();
      lastClockFaceTime = currentMillis;
    }
  }

  // useStaticImage
  if (_mode == ALBUM) {
    // Static image mode - load and display static1.h file once
    static bool imageDisplayed = false;
    if (!imageDisplayed) {
      // Ensure sprite is created
      if (!sprite.created()) {
        Serial.println("Sprite not created, creating now...");
        sprite.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
        sprite.setPivot(SPRITE_WIDTH/2, SPRITE_HEIGHT/2);
      }
      
      Serial.println("Displaying static image...");
      loadImageToSpriteFromLittleFS(sprite, "/static1.rgb565", 0, 0);
      
      // Draw text on the sprite BEFORE rotating
      sprite.setFreeFont(&GilliganShutter10pt7b);  // Set the custom font on sprite
      sprite.setTextColor(TFT_WHITE, TFT_BLACK);   // White text, black background
      sprite.setTextDatum(MC_DATUM);               // Middle center alignment
      //sprite.drawString("As long as you live...", SPRITE_WIDTH / 2, SPRITE_HEIGHT / 2);
      
      // Now push the sprite with text rotated 90 degrees
      sprite.pushRotated(90);
      Serial.println("Static image displayed.");
      
      imageDisplayed = true;
    }
  }


  // Check if it's time to fetch a new image (if not using static image)
  if (_mode == FETCH) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastFetchTime >= fetchInterval) {
      fetchAndDisplayImage();
      // While developing, just write some text instead of fetching image
      // tft.setTextColor(TFT_YELLOW);
      // tft.setTextSize(2);
      // tft.setCursor(10, 10);
      // tft.println("Dev mode...");
      lastFetchTime = currentTime;
    }
  }

  //fetchAndDisplayImage();

  // Print a dot to serial every other second or so
  static unsigned long lastDotTime = 0;
  unsigned long currentTime = millis();
  if (currentTime - lastDotTime >= 2000) {
    lastDotTime = currentTime;
    Serial.print(".");
  }


  // Poll button for 200ms instead of simple delay
  unsigned long buttonPollStart = millis();
  while (millis() - buttonPollStart < 200) {
    if (digitalRead(PIN_BTN1) == LOW) {
      btn1Pressed();
      // Wait for button release with timeout
      unsigned long releaseWait = millis();
      while (digitalRead(PIN_BTN1) == LOW && (millis() - releaseWait < 1000)) {
        delay(10);
      }
      break; // Exit polling loop after handling press
    }
    delay(10); // Small delay between checks
  }
}

void btn1Pressed() {
  Serial.println("Button 1 pressed");

  // Wait for button release
  while (digitalRead(PIN_BTN1) == LOW) {
    delay(10);
  }
  delay(200); // Debounce delay before entering menu

  // Clear display
  tft.fillScreen(TFT_BLACK);

  byte lineHeight = 30;
  // Draw menu: Fetch Image, Show Clock, Show Quote, Show Status
  // enum MODE { FETCH, CLOCK, ALBUM, QUOTE, STATUSINFO, NONE  };

  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(20, lineHeight * 1);
  tft.println("Menu:");
  tft.setCursor(40, lineHeight * 2);
  tft.println("1. Fetch Image");
  tft.setCursor(40, lineHeight * 3);
  tft.println("2. Show Clock");
  tft.setCursor(40, lineHeight * 4);
  tft.println("3. Show Album");
  tft.setCursor(40, lineHeight * 5  );
  tft.println("4. Show Quote");
  tft.setCursor(40, lineHeight * 6  );
  tft.println("5. Show Status");

  // Initialize position based on current mode
  byte position = (_mode >= FETCH && _mode <= STATUSINFO) ? static_cast<byte>(_mode) : 0;
  
  while (true) {
    // Highlight current selection
    for (int i = 0; i < 5; i++) {
      if (i == position) {
        tft.setTextColor(TFT_DARKGREEN, TFT_GREENYELLOW);
      } else {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
      }

      // enum MODE { FETCH, CLOCK, ALBUM, QUOTE, STATUSINFO, NONE  };

      tft.setCursor(40, lineHeight * (i + 2));
      switch (i) {
        case 0: tft.println("1. Fetch Image"); break;
        case 1: tft.println("2. Show Clock"); break;
        case 2: tft.println("3. Show Album"); break;
        case 3: tft.println("4. Show Quote"); break;
        case 4: tft.println("5. Show Status"); break;
      }
    }

    // Check button 1 press to move selection
    if (digitalRead(PIN_BTN1) == LOW) {
      // Move to next position
      position = (position + 1) % 5;
      // Wait for button release
      while (digitalRead(PIN_BTN1) == LOW) {
        delay(10);
      }
      delay(200); // Debounce delay
    }

    // Check button 2 press to select current option
    if (digitalRead(PIN_BTN2) == LOW) {
      // Button 2 pressed, make selection
      _mode = static_cast<MODE>(position); 
      Serial.print("Selected mode: "); Serial.println(_mode);
      tft.fillScreen(TFT_BLACK);
      if (_mode == FETCH) {
        //tft.fillScreen(TFT_BLACK); // Clear screen for fetch mode
        // Write a status text before fetching image
        tft.setTextColor(TFT_YELLOW);
        tft.setTextSize(2);
        tft.setCursor(10, 10);
        tft.println("Fetching image...");
        lastFetchTime = 99999999; // Force immediate update on mode change
      }
      

      // Wait for button release
      while (digitalRead(PIN_BTN2) == LOW) {
        delay(10);
      }


      // switch(_mode) {
      //   case FETCH:
      //    GetNinjaQuote(); // Pre-fetch quote
      // }



      return;
    }
    
    delay(100); // Polling delay
  }
  Blip(1);
}


void Blip(int blips){
  for (int i = 0; i < blips; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(100);
    digitalWrite(PIN_LED, LOW);
    delay(300);
  }
}

// Base64 decoding lookup table
const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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

// Draw RGB565 image array to display
// imageData: pointer to RGB565 array (uint16_t)
// width, height: dimensions of the image
// xpos, ypos: position on screen
void drawRGB565Image(const uint16_t* imageData, int width, int height, int xpos, int ypos) {
  if (imageData == NULL) {
    Serial.println("ERROR: NULL image data");
    return;
  }
  
  Serial.print("Drawing RGB565 image: ");
  Serial.print(width);
  Serial.print("x");
  Serial.print(height);
  Serial.print(" at (");
  Serial.print(xpos);
  Serial.print(", ");
  Serial.print(ypos);
  Serial.println(")");
  
  // Push the image directly to TFT (reads from PROGMEM if needed)
  tft.pushImage(xpos, ypos, width, height, imageData);
  
  Serial.println("RGB565 image displayed successfully");
}

// Draw RGB565 image array to sprite
// sprite: reference to sprite object
// imageData: pointer to RGB565 array (uint16_t)
// width, height: dimensions of the image
// xpos, ypos: position within sprite
void drawRGB565ImageToSprite(TFT_eSprite &spr, const uint16_t* imageData, int width, int height, int xpos, int ypos) {
  if (imageData == NULL) {
    Serial.println("ERROR: NULL image data");
    return;
  }
  
  // Push the image to sprite (reads from PROGMEM if needed)
  spr.pushImage(xpos, ypos, width, height, imageData);
}

// Decode Base64 JPEG and display it
void drawJpegFromBase64(const char* base64String, int xpos, int ypos) {
  // Just return if string is empty
  if (base64String == NULL || strlen(base64String) == 0) {
    Serial.println("ERROR: Empty Base64 string");
    return;
  }

  // Calculate approximate decoded size (Base64 is ~133% of original)
  //Serial.println("-----2");

  size_t base64Len = strlen(base64String);
  //Serial.println("-----3");
  size_t maxDecodedSize = (base64Len * 3) / 4 + 1;
  //Serial.println("-----4");
  
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

// Render the decoded JPEG to the TFT display (simplified - assumes correct size)
void renderJPEG(int xpos, int ypos) {
  Serial.print("JPEG dimensions: ");
  Serial.print(JpegDec.width);
  Serial.print(" x ");
  Serial.println(JpegDec.height);
  
  // Read and display each MCU block
  // Use readSwappedBytes() for proper RGB565 format with byte-swapping
  while (JpegDec.readSwappedBytes()) {
    int mcu_x = JpegDec.MCUx * JpegDec.MCUWidth + xpos;
    int mcu_y = JpegDec.MCUy * JpegDec.MCUHeight + ypos;
    
    // Rotate the image 180 degrees before pushing
    // Calculate rotated position
    int rotated_x = JpegDec.width - (JpegDec.MCUx + 1) * JpegDec.MCUWidth + xpos;
    int rotated_y = JpegDec.height - (JpegDec.MCUy + 1) * JpegDec.MCUHeight + ypos;

    tft.pushImage(rotated_x, rotated_y, JpegDec.MCUWidth, JpegDec.MCUHeight, JpegDec.pImage);
  }
}

// Connect to WiFi
void connectWiFi() {
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(wifiSSID);
  
  // Configure static IP
  if (!WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
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
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
  }
}

// Fetch image from server and display it
void fetchAndDisplayImage() {
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
        Serial.print("JpegDec error: ");
        //Serial.println(JpegDec.getLastError());
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



void DrawClockFace() {
  DrawClockFace(CLOCK_WIDTH / 2, CLOCK_HEIGHT / 2, (CLOCK_WIDTH / 2) * CLOCK_SCALE);
}

// void UpdateClockFace() {
//   UpdateClockFace(CLOCK_SCALE);
// }

void DrawClockFace(int xpos, int ypos, float scale) {
  // Draw clock face on sprite (sprite should already be cleared/filled by caller)
  sprite.setTextColor(TFT_WHITE); // White text
  const float indlen = 0.9; // Indicator length factor
  // Scale text size proportionally
  //sprite.setTextSize(max(1, (int)(2 * CLOCK_SCALE)));
  // Use fixed size insted
  sprite.setTextSize(1);
  // Use fancy font
  sprite.setFreeFont(&GilliganShutter10pt7b);  // Set the custom font on sprite
  sprite.setTextDatum(MC_DATUM); // Set text datum to Middle-Center for proper centering
  
  for (int i = 0; i < 12; i++) {
      const float angle = (i / 12.0) * 2 * 3.1416 - 3.1416 / 2;
      const int x = xpos + cos(angle) * scale * 1.2;
      const int y = ypos + sin(angle) * scale * 1.2;
      const int xm = xpos + cos(angle) * scale * 0.85;
      const int ym = ypos + sin(angle) * scale * 0.85;

      //sprite.drawString(String(i == 0 ? 12 : i), x, y);
      // Translate i to roman numeral
      String romanNumeral;
      switch (i) {
        case 0: romanNumeral = "XII"; break;
        case 1: romanNumeral = "I"; break;
        case 2: romanNumeral = "II"; break;
        case 3: romanNumeral = "III"; break;
        case 4: romanNumeral = "IV"; break;
        case 5: romanNumeral = "V"; break;
        case 6: romanNumeral = "VI"; break;
        case 7: romanNumeral = "VII"; break;
        case 8: romanNumeral = "VIII"; break;
        case 9: romanNumeral = "IX"; break;
        case 10: romanNumeral = "X"; break;
        case 11: romanNumeral = "XI"; break;
      }
      // Draw centered text - MC_DATUM centers both horizontally and vertically
      sprite.drawString(romanNumeral, x, y);


      const int ix = xpos + cos(angle) * scale * indlen;
      const int iy = ypos + sin(angle) * scale * indlen;
      //sprite.drawLine(xpos + cos(angle) * (scale - ix), ypos + sin(angle) * (scale - iy), x, y, TFT_LIGHTGREY);
      // Scale line widths proportionally
      // EA 2025-11-23 Modify scale for marker lines, we want them much further out
      if (i % 3 == 0)
        sprite.drawWideLine(xm, ym, ix, iy, max(1, (int)(5 * CLOCK_SCALE)), TFT_LIGHTGREY);
      else  
        sprite.drawWideLine(xm, ym, ix, iy, max(1, (int)(3 * CLOCK_SCALE)), TFT_LIGHTGREY);

      // if (i % 3 == 0)
      //   sprite.drawWideLine(x, y, ix, iy, max(1, (int)(3 * CLOCK_SCALE)), TFT_LIGHTGREY);
      // else  
      //   sprite.drawLine(x, y, ix, iy, TFT_LIGHTGREY);
  } 
}

void UpdateClockFace() {
  // Ensure sprite is created
  if (!sprite.created()) {
    Serial.println("Sprite not created, creating now...");
    sprite.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    sprite.setPivot(SPRITE_WIDTH/2, SPRITE_HEIGHT/2);
  }
  
  // Get current time
  time_t now = time(nullptr);
  localtime_r(&now, &timeinfo);

  //timeinfo.tm_sec += 1;

  int hours = timeinfo.tm_hour;
  int minutes = timeinfo.tm_min;
  int seconds = timeinfo.tm_sec;

  // Calculate angles
  float secondAngle = (seconds / 60.0) * 2 * 3.1416 - 3.1416 / 2;
  float minuteAngle = (minutes / 60.0) * 2 * 3.1416 - 3.1416 / 2 + (secondAngle + 3.1416 / 2) / 60.0;
  float hourAngle = ((hours % 12) / 12.0) * 2 * 3.1416 - 3.1416 / 2 + (minuteAngle + 3.1416 / 2) / 12.0;

  // Load background image from LittleFS to sprite
  // Note: This loads line-by-line so it's memory efficient
  loadImageToSpriteFromLittleFS(sprite, "/backclock.rgb565", 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT);
  
  // Calculate clock position in full-screen sprite (centered)
  int clockCenterX = SPRITE_WIDTH / 2;
  int clockCenterY = SPRITE_HEIGHT / 2;
  // Apply scale to clock radius
  float clockRadius = (CLOCK_WIDTH / 2) * CLOCK_SCALE;

  // Draw clock face (numbers and hour markers)
  DrawClockFace(clockCenterX, clockCenterY, clockRadius);

  // Draw hour hand on sprite (scaled)
  int hourX = clockCenterX + cos(hourAngle) * (clockRadius * 0.5);
  int hourY = clockCenterY + sin(hourAngle) * (clockRadius * 0.5);
  sprite.drawWideLine(clockCenterX, clockCenterY, hourX, hourY, max(1, (int)(5 * CLOCK_SCALE)), TFT_WHITE);

  // Draw minute hand on sprite (scaled)
  int minuteX = clockCenterX + cos(minuteAngle) * (clockRadius * 0.7);
  int minuteY = clockCenterY + sin(minuteAngle) * (clockRadius * 0.7);
  sprite.drawWideLine(clockCenterX, clockCenterY, minuteX, minuteY, max(1, (int)(3 * CLOCK_SCALE)), TFT_WHITE);

  // Draw second hand on sprite (scaled)
  int secondX = clockCenterX + cos(secondAngle) * (clockRadius * 0.9);
  int secondY = clockCenterY + sin(secondAngle) * (clockRadius * 0.9);
  sprite.drawLine(clockCenterX, clockCenterY, secondX, secondY, TFT_RED);

  // Push the entire full-screen sprite to display with 90° rotation
  //tft.setPivot(TFT_WIDTH / 2, TFT_HEIGHT / 2); // Set pivot to center of display
  sprite.pushRotated(90); // Rotate 90 degrees clockwise - no transparency needed now
}

// Load RGB565 image from LittleFS and display on TFT
void loadImageFromLittleFS(const char* filename, int xpos, int ypos) {
  Serial.print("Loading image from LittleFS: ");
  Serial.println(filename);
  
  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open image file");
    return;
  }
  
  size_t fileSize = file.size();
  Serial.print("File size: ");
  Serial.print(fileSize);
  Serial.println(" bytes");
  
  // Allocate buffer for image data
  uint16_t* buffer = (uint16_t*)malloc(fileSize);
  if (!buffer) {
    Serial.println("Failed to allocate memory for image");
    file.close();
    return;
  }
  
  // Read file into buffer
  size_t bytesRead = file.read((uint8_t*)buffer, fileSize);
  file.close();
  
  if (bytesRead != fileSize) {
    Serial.println("Failed to read complete file");
    free(buffer);
    return;
  }
  
  // Calculate dimensions (assuming 320x240 or derive from fileSize)
  int width = 320;
  int height = fileSize / (width * 2);
  
  Serial.print("Image dimensions: ");
  Serial.print(width);
  Serial.print("x");
  Serial.println(height);
  
  // Push image to display
  tft.pushImage(xpos, ypos, width, height, buffer);
  
  free(buffer);
  Serial.println("Image loaded successfully");
}

// Load RGB565 image from LittleFS and draw to sprite (line-by-line to save RAM)
// imageWidth and imageHeight are optional - if imageHeight is 0, it's calculated from file size
// transparentColor: pixels of this color won't be drawn (default 0xFFFF = white, use 0xF81F for magenta)
void loadImageToSpriteFromLittleFS(TFT_eSprite &spr, const char* filename, int xpos, int ypos, int imageWidth, int imageHeight, uint16_t transparentColor) {
  //Serial.print("Loading image to sprite from LittleFS: ");
  //Serial.println(filename);
  
  // Ensure sprite is created
  if (!spr.created()) {
    Serial.println("Sprite not created, creating now...");
    spr.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    spr.setPivot(SPRITE_WIDTH/2, SPRITE_HEIGHT/2);
  }
  
  File file = LittleFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open image file");
    return;
  }
  
  size_t fileSize = file.size();
  //Serial.print("File size: ");
  //Serial.print(fileSize);
  //Serial.println(" bytes");
  
  // Calculate dimensions - use provided width, calculate height if not provided
  int width = imageWidth;
  int height = imageHeight;
  if (height == 0) {
    height = fileSize / (width * 2);
  }
  
  //Serial.print("Image dimensions: ");
  //Serial.print(width);
  //Serial.print("x");
  //Serial.println(height);
  
  // Check if transparency is enabled
  bool useTransparency = (transparentColor != 0xFFFF); // 0xFFFF means no transparency
  // if (useTransparency) {
  //   Serial.print("Using transparency: 0x");
  //   Serial.println(transparentColor, HEX);
  // }
  
  // Allocate buffer for ONE line (width pixels * 2 bytes per pixel)
  size_t lineBufferSize = width * sizeof(uint16_t);
  uint16_t* lineBuffer = (uint16_t*)malloc(lineBufferSize);
  if (!lineBuffer) {
    Serial.println("Failed to allocate line buffer");
    file.close();
    return;
  }
  
  long totalBytesRead = 0;
  // Read and push image line by line
  for (int y = 0; y < height; y++) {
    size_t bytesRead = file.read((uint8_t*)lineBuffer, lineBufferSize);
    if (bytesRead != lineBufferSize) {
      Serial.print("Failed to read line ");
      Serial.print(y);
      Serial.print(" - Expected ");
      Serial.print(lineBufferSize);
      Serial.print(" bytes, got ");
      Serial.println(bytesRead);
      break;
    }
    // Push one line at a time to sprite with optional transparency
    if (useTransparency) {
      // Draw pixel by pixel, skipping transparent color
      for (int x = 0; x < width; x++) {
        if (lineBuffer[x] != transparentColor) {
          spr.drawPixel(xpos + x, ypos + y, lineBuffer[x]);
        }
      }
    } else {
      // No transparency - push entire line at once (faster)
      spr.pushImage(xpos, ypos + y, width, 1, lineBuffer);
    }
    totalBytesRead += bytesRead;
  }
  
  //Serial.print("Total bytes read: ");
  //Serial.println(totalBytesRead);
  
  file.close();
  free(lineBuffer);
  // Serial.println("Image loaded to sprite successfully");
}

// Function GetNinjaQuote(), return malloc'd char array (CALLER MUST FREE!)
// Returns nullptr on error
char* GetNinjaQuote() {
  // Setup GET request to quote API using WiFiClientSecure
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation
  
  const char* host = "api.api-ninjas.com";
  const int httpsPort = 443;
  
  Serial.println("Connecting to API Ninjas...");
  if (!client.connect(host, httpsPort)) {
    Serial.println("ERROR: Failed to connect to quote API");
    return nullptr;
  }
  
  // Send HTTP GET request (fixed typo: GET not GE4T)
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
  StaticJsonDocument<512> doc;
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

void drawWrappedText(TFT_eSprite &spr, const char* text, int x, int y, int maxWidth, int lineHeight) {
  spr.setFreeFont(&GilliganShutter10pt7b);
  spr.setTextColor(TFT_WHITE);
  spr.setTextDatum(TL_DATUM);  // Top-left alignment
  
  char buffer[256];
  strncpy(buffer, text, sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';
  
  char* word = strtok(buffer, " ");
  int currentX = x;
  int currentY = y;
  
  while (word != NULL) {
    int wordWidth = spr.textWidth(word);
    int spaceWidth = spr.textWidth("  ");
    
    // Check if word fits on current line
    if (currentX + wordWidth > x + maxWidth && currentX > x) {
      // Move to next line
      currentX = x;
      currentY += lineHeight;
    }
    
    // Draw the word
    spr.setCursor(currentX, currentY);
    spr.print(word);
    currentX += wordWidth + spaceWidth;
    
    word = strtok(NULL, " ");
  }
}

void displayStatusInfo() {
  Serial.println("Displaying status info...");
  
  // Clear screen
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextDatum(TL_DATUM);  // Top-left alignment
  
  int y = 10;
  int lineHeight = 16;
  
  // Version
  tft.setCursor(10, y);
  tft.print("Version: ");
  tft.println(VERSION);
  y += lineHeight;
  
  // Uptime
  tft.setCursor(10, y);
  unsigned long uptimeSeconds = millis() / 1000;
  unsigned long days = uptimeSeconds / 86400;
  unsigned long hours = (uptimeSeconds % 86400) / 3600;
  unsigned long minutes = (uptimeSeconds % 3600) / 60;
  unsigned long seconds = uptimeSeconds % 60;
  tft.printf("Uptime: %dd %02d:%02d:%02d\n", days, hours, minutes, seconds);
  y += lineHeight;
  
  // WiFi Status
  tft.setCursor(10, y);
  tft.print("WiFi: ");
  tft.println(WiFi.isConnected() ? "Connected" : "Disconnected");
  y += lineHeight;
  
  // IP Address
  tft.setCursor(10, y);
  tft.print("IP: ");
  tft.println(WiFi.localIP().toString());
  y += lineHeight;
  
  // WiFi Signal Strength
  tft.setCursor(10, y);
  int rssi = WiFi.RSSI();
  tft.print("Signal: ");
  tft.print(rssi);
  tft.print(" dBm (");
  // Convert RSSI to quality percentage
  int quality;
  if (rssi <= -100) quality = 0;
  else if (rssi >= -50) quality = 100;
  else quality = 2 * (rssi + 100);
  tft.print(quality);
  tft.println("%)");
  y += lineHeight;
  
  // Memory Usage
  tft.setCursor(10, y);
  tft.print("Free Heap: ");
  tft.print(ESP.getFreeHeap() / 1024);
  tft.print(" / ");
  tft.print(ESP.getHeapSize() / 1024);
  tft.println(" KB");
  y += lineHeight;
  
  tft.setCursor(10, y);
  tft.print("Min Free: ");
  tft.print(ESP.getMinFreeHeap() / 1024);
  tft.println(" KB");
  y += lineHeight;
  
  // Flash Memory
  tft.setCursor(10, y);
  tft.print("Flash: ");
  tft.print(ESP.getFlashChipSize() / 1024 / 1024);
  tft.println(" MB");
  y += lineHeight;
  
  // LittleFS Information
  tft.setCursor(10, y);
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  tft.printf("LittleFS: %d / %d KB\n", usedBytes / 1024, totalBytes / 1024);
  y += lineHeight;
  
  // Count files in LittleFS
  tft.setCursor(10, y);
  int fileCount = 0;
  File root = LittleFS.open("/");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        fileCount++;
      }
      file = root.openNextFile();
    }
  }
  tft.print("Files: ");
  tft.println(fileCount);
  y += lineHeight;
  
  // Current Mode
  y += 5; // Extra spacing
  tft.setCursor(10, y);
  tft.print("Mode: ");
  switch(_mode) {
    case FETCH: tft.println("FETCH"); break;
    case ALBUM: tft.println("ALBUM"); break;
    case QUOTE: tft.println("QUOTE"); break;
    case CLOCK: tft.println("CLOCK"); break;
    case STATUSINFO: tft.println("STATUSINFO"); break;
    case NONE: tft.println("NONE"); break;
  }
} 