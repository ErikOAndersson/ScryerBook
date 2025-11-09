#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoHttpClient.h>
#include <Preferences.h>
//#include "beach.h"
#include "wifi_credentials.h"

#define min(a,b)     (((a) < (b)) ? (a) : (b))

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

// Image server configuration - can be changed via web interface
String imageServerAddress = "witty-cliff-e5fe8cff64ae48afb25807a538c2dad2.azurewebsites.net";
int imageServerPort = 443;  // HTTPS port
String imageServerPath = "/?width=240&height=320&rotate=90";
bool useStaticImage = false;

// Create HTTPS client
HttpClient client = HttpClient(wifi, imageServerAddress.c_str(), imageServerPort);

// Timing - can be changed via web interface
unsigned long lastFetchTime = 0;
unsigned long fetchInterval = 20000; // 20 seconds (default)

// Function declarations
void drawSpots();
void Blip(int blips);
void drawJpegFromBase64(const char* base64String, int xpos, int ypos);
void renderJPEG(int xpos, int ypos);
size_t base64Decode(const char* input, uint8_t* output, size_t outputSize);
void connectWiFi();
void fetchAndDisplayImage();
void loadSettings();
void saveSettings();
void handleWebServer();
String urlDecode(String str);
unsigned char h2int(char c);


TFT_eSPI tft = TFT_eSPI();  

 // do not forget User_Setup_xxxx.h matching microcontroller-display combination
 // my own: "/include/User_Setup.h"

#define PIN_LED 21

int delayTime = 500;
int j,t;
int count = 0;

// colors are:       bordeaux-africa - green - blue  - red  - yellow -bordeaux- africa - green - blue  - red  - yellow -bordeaux in hex, see defines above
int color [13] = {  0xA000, 0xAB21, 0x07E0, 0x001F, 0xF800, 0xFFE0,  0xA000,  0xAB21,  0x07E0, 0x001F, 0xF800, 0xFFE0, 0xA000};

//const char beach_b64[] PROGMEM = "";

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Blip(10);

  Serial.begin (9600);
  delay(1000);  // Give serial time to initialize

  // Load saved settings from preferences
  loadSettings();

  Serial.println("Initializing TFT...");
  Serial.print("TFT_CS: "); Serial.println(TFT_CS);
  Serial.print("TFT_DC: "); Serial.println(TFT_DC);
  Serial.print("TFT_MOSI: "); Serial.println(TFT_MOSI);
  Serial.print("TFT_SCLK: "); Serial.println(TFT_SCLK);
  Serial.print("TFT_RST: "); Serial.println(TFT_RST);
  
  delay(100);  // Let pins stabilize

  // Connect to WiFI with static IP
  connectWiFi();
  
  // Start web server
  webServer.begin();
  Serial.println("Web server started");
  Serial.print("Access settings at: http://");
  Serial.println(WiFi.localIP());
  
  // Configure WiFiClientSecure to skip certificate validation (for development)
  // For production, you should use proper certificate validation
  wifi.setInsecure();

  tft.begin();
  Serial.println("TFT initialized successfully!");
  //tft.fillScreen (BLACK);
  Serial.println ();                                 // cut gibberish in Serial Monitor
  Serial.println ();    
  Serial.println ("Colors array - GC9A01 display");  // test Serial Monitor
  Serial.println ("on ESP32-C3 Super Mini");  
  //Serial.println ();
  // tft.drawCircle (120,120,80,SCOPE);
  tft.fillScreen(TFT_BLACK);
  j=12; 

  Serial.println ("Starting up...");
  
  // Example: Display JPEG from Base64 string
  // Uncomment to test with your Base64-encoded JPEG
  
  //const char* base64Jpeg = "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AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMj9qK/Ff/hqj9p7/AKOO8ef+Fhe//HaP+KR/iL/0UuE/8E1v8w/4lpzz/oPp/wDgMjg6K/3aP7CCigAooAKKACigAooAKKACigAooAKKACigAooAKKACvpz/AIJqf8Em/wBqP/gqj4l8V6F+zrqHhXS7XwXY2s+va14w1aW2to5Ll5Ft7dVghnmeSQQ3DgiPy1WBt7qzRq/yfFXHXB/A9KlUz7HU8Mqrahzys5ctnLlWrajdczSsuaKbTkr9FDC4jFNqlFu29j5jr9U/+IQr/gpP/wBFu+B3/hSax/8AKmvjP+I/eDf/AEOqP3v/AOROj+ysw/59s/Kyv1T/AOIQr/gpP/0W74Hf+FJrH/ypo/4j94N/9Dqj97/+RD+ysw/59s/Kyvv79ob/AINmv+CsvwH+1X2g/CHQ/iPpdjob6leap8PPEkU+zZ5he2S1vBbXlxcBYwwjggk3+aioXclF9fLPGDwrziywud4Vty5VGVaEJOTtZKM3GTvdJNJpvRapoznl+Op/FTl9zf5HwDV7xP4Y8S+CfEuoeDPGfh6+0jWNIvprLVtJ1S0e3ubK5icpLBLE4DRyI6srIwDKwIIBFfolGtSxFKNWlJSjJJpp3TT1TTWjTWqa3ONpp2ZRorQAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigD9xP+DMz/AJuQ/wC5P/8Ac3R/wZmf83If9yf/AO5uv4B+nN/zT/8A3N/+6x9Vwz/y9/7d/wDbj9Gv+CvX/BTP/h1R+zXof7Q3/Ck/+E8/tnxxbeHf7H/4ST+y/J82zvLnz/N+zXG7H2Tbs2DPmZ3Dbg/K3/B3r/yjY8Ef9lx03/0z6zX4h9F/gLhPxE4+xOW8Q4b29GGFnUUeepC01VoxTvTlCTtGclZu2t7XSt6WdYrEYTCxnSdnzJbJ6Wfc8O/4jM/+sb//AJmD/wC9Ffh3X93f8SueBP8A0KP/AC4xX/y8+Y/tvNP+fn4R/wAj+sL/AIJef8FtP2Tf+CpHmeCPhvZa54Z+Imm6GdT17wPrli8nk28f2SOe4t72JTBPbi4u1hQuYZ32lzbxrg1/OX/wSD+JPjT4Vf8ABUb4BeJ/AWs/YL66+KujaPPP9njl3WWo3SafeRYkVlHmWtzPHuA3Lv3IVYKw/EPGP6J/BuV8I47PeGas6FTDQnWdKc3OlKnTUpzjFuLqRny/A3OcXyqEkud1I+nl+e4ipiI0qyTUna60d3ovK3f+kfvB/wAHIH/BMvwX+2T+xvrH7S3hi20PR/iJ8HtDvNcHiC8tZBNqnh+0t57i70lpIj9biAyJIElRo18lbqaUfo1X8g+F3izxX4VZ5DGZbVk6DkvbUHL93VjdOSs1JQm0rRqqPPDVaxcoy9/HYGhjqfLNa9H1X/A8j+Heiv8Aaw/OAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigD9xP+DMz/m5D/uT/AP3N0f8ABmZ/zch/3J//ALm6/gH6c3/NP/8Ac3/7rH1XDP8Ay9/7d/8Abj3H/g71/wCUbHgj/suOm/8Apn1mvv8A/a9/bZ/Zi/YN+Gtj8X/2r/iZ/wAIr4d1LXI9Hs9Q/sW9vvMvZIZpki8uzhlkGY7eZtxUKNmCQSAf54+j1xtn/AfGmIx+UZRVzOpLDzpulS5+aMXUpSdR8lKq+VOKi/dSvNe8nZP1s2w1LFYdQqVFBXvd+j01aP4y6/qm/wCIkP8A4Iu/9Hl/+Y78R/8Ayvr+x/8AiY7xT/6IHG/fX/8AmI+f/sfA/wDQVH8P/kj8uv8Ag3V/4It/G/4v/tG+E/26v2ivh3feGvhv4GvrXXfCdt4gs7m0ufFeo+Stxp9zaKGjb7HC72939qO6GZkjhRZlacw/tR+yx/wVP/4J6ftqX8Ohfs2/tYeFde1i6vprSy8N3dxJpurXckMAuJGhsL5IbqaNYsuZY42jxHJ82Y3C/wA/+M3jp408RZTWyvF5VVyrB1I2qJ06sak6cpWUZ1akYWhLSD5Iw5/ejJuEnA9TL8sy6jUU4zU5LbVWT9F19b2/E8q/4Lxf8FE/DX/BPv8AYS1+Sz1G+Hjr4kWN94X8Aw6JryWN/Y3M1pIsmro4bzljswySeZCrMJ5LWMtF5wlT5y/4LDf8G2vhr9rO/wDiB+19+y58SfFU3xi1y+j1VvCvizxEl1pOqrHAI3sbaWdPOs5GCIYfNne2iKi3CW8BR7fwPAXLfAmjn+Ex3FGZy+sK0o0atJ0qFOrGacXOsqkozjopJTVOm1dVL35Htmk80dKUaENO6d215K2nyu+x/OxV7xP4Y8S+CfEuoeDPGfh6+0jWNIvprLVtJ1S0e3ubK5icpLBLE4DRyI6srIwDKwIIBFf6s0a1LEUo1aUlKMkmmndNPVNNaNNaprc+FaadmUaK0AKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigD9xP8AgzM/5uQ/7k//ANzdH/BmZ/zch/3J/wD7m6/gH6c3/NP/APc3/wC6x9Vwz/y9/wC3f/bj3H/g71/5RseCP+y46b/6Z9Zo/wCDvX/lGx4I/wCy46b/AOmfWa/P/oV/8nTxn/YFU/8AT+GOriP/AHGP+Jfkz+cqiv8AT4+LL3hjxP4l8E+JdP8AGfgzxDfaRrGkX0N7pOraXdvb3NlcxOHinilQho5EdVZXUhlYAggiqNZ1qNLEUpUqsVKMk001dNPRpp6NNaNPcE3F3R/Vp/wQE/bl8S/t4f8ABN7wz4z+IU19deLPA99J4N8VatfM7tqtzZwQPFeebJNLLPJLaXFq00shVnuTcEIqbM8P/wAGwn7OXiX4Af8ABKrRPEXit76G6+JnirUPF8Wm6hpD2kljbSLBY24G9iZo5oLCK7jm2orR3abQygSP/kd9KDLODcp8WK9Dh5Rj7kZV4xuowxEnJzUVZRV4OnKSjeKnKS0kpRj99ks8RUwKdXvp6f8AD3Pzk/4O4P2WLD4Wftr+Dv2o9As7G3tfix4VeDVljvZ5Lm41bSTFBLcSI4McUZsrjTIkETAM1vKzIrHfJ3P/AAeOfF/w1rXxv+CPwDtbG+XWPDXhXV/EF9cyRoLaS21K5gt4EjYOWMivpNwXBRVCvFtZiWCf0/8AQozPiTF8I5lhMU5SwVGrBUG7tRnJSlXhFt6RX7qfKkkpVJS1c5Hi8SQoxxEJR+Jp39Oj/NfLyPxlor+1j5sKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigAooAKKACigD9xP+DMz/AJuQ/wC5P/8Ac3X5Pfsdf8FAf2wv2AvEus+K/wBkb4233hC68Q2MdprcUdja3ttexxvvjaS3u4pYTIhL7JdnmIssqqwWWQN/Mn0j/BPiTxgpZY8nr0acsK63MqznFSVX2VmnCFR3Tp7NK9730s/ayfMqOXuftE3zW2t0v3a7n9U3/BRz/gnH8EP+CnvwQ0v4B/HzxT4q0jR9I8VQeILa58H31tb3LXMVtc26ozXFvOpj2XUhICBtwX5gAQf52P8AiJD/AOC0X/R5f/mO/Dn/AMr6/AOF/os+PHBeYSx2R5thcPVlFwcoVaybg3GTjrhno3GL+SPUrZ5leIjy1acmt9l/8kfqn/xCFf8ABNj/AKLd8cf/AApNH/8AlTX5Wf8AESH/AMFov+jy/wDzHfhz/wCV9fef8Qn+lv8A9FPR/wDBtX/5lOb69kP/AD5f3L/5I/aj9lD/AINpf+CXP7LPjSbx7qHgHXPijfDb/Z8HxYvbXUrKx/dzRybbOG2gt596y5P2mObY0UbxeWylj+Fvxf8A+C8H/BXT43+GoPCnjP8Abj8VWVrb3y3ccvg+2s/D1yZFR0CtcaVBbzPHiRsxM5jLBWKlkQrx4vwH+kpxFH6vnXFMVSttTrV3d3i0pRVKkpJWunJy5WtFq2VHNMno606Gvml/mz+jT/gpb/wVc/Zc/wCCZPwm1DxX8VPFVjq/jR7FJfCvwx0/VYl1bWZJTKkMhj+Zraz3wy+ZeOhjQROqiWUxwyfyQ+J/E/iXxt4l1Dxn4z8Q32r6xq99Ne6tq2qXb3Fze3Mrl5Z5ZXJaSR3ZmZ2JZmJJJJr1OD/oWcK5biKdfiLHzxdtXThH2NNuy92UlKVSUU+bWMqcn7r92zTjEcR15xaoxUfN6v8Ay/M7j9rj9qf4s/tsftG+Kv2o/jjeWM3ibxdfJPfLpdkLe2t444Uggt4kBJEcUEUUSl2eRljDSPI5Z285r+wslyXKOHMrpZbldCNGhTVowglGKu23ZLq23KTespNybbbZ8/UqVK03Obu31YUV6hmFFABRQAUUAFFABRQAUUAFFABRQAUUAFFABRQAUUAFFABRQAUUAFFABRQAUUAFFABRQAUUAFFABRQAUUAFFABRQAUUAFFABRQAUUAFFABRQAUUAFFABRQAUUAFFABRQAUUAFFAH//Z";
  const char* base64Jpeg = "";
  tft.invertDisplay(true); // Invert display colors
  //tft.setSwapBytes(true); // Swap byte order for TFT
  tft.setSwapBytes(false); // This is back again? Trying default.
  tft.fillScreen(TFT_BLACK);

  //http://witty-cliff-e5fe8cff64ae48afb25807a538c2dad2.azurewebsites.net/?width=320&height=240&info&rotate=90
}

void loop() {
  // Handle web server requests
  handleWebServer();
  
  // Check if it's time to fetch a new image (if not using static image)
  if (!useStaticImage) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastFetchTime >= fetchInterval) {
      //fetchAndDisplayImage();
      // While developing, just write some text instead of fetching image
      tft.setTextColor(TFT_YELLOW, TFT_PINK);
      tft.setTextSize(2); 
      tft.setCursor(10, 10);
      tft.println("Dev mode...");
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


  // Small delay to prevent tight loop
  delay(100);
}


void drawSpots(){
 
   tft.fillCircle (120,  40, 8, color [j]); 
   tft.fillCircle (190,  80, 8, color [j-1]);  
   tft.fillCircle (190, 160, 8, color [j-2]);  
   tft.fillCircle (120, 200, 8, color [j-3]);   
   tft.fillCircle ( 50, 160, 8, color [j-4]); 
   tft.fillCircle ( 50,  80, 8, color [j-5]);                         
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
  while (JpegDec.read()) {
  //while (JpegDec.readSwappedBytes()) {
    int mcu_x = JpegDec.MCUx * JpegDec.MCUWidth + xpos;
    int mcu_y = JpegDec.MCUy * JpegDec.MCUHeight + ypos;
    
    tft.pushImage(mcu_x, mcu_y, JpegDec.MCUWidth, JpegDec.MCUHeight, JpegDec.pImage);
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
    return;
  }
  
  // Skip if using static image
  if (useStaticImage) {
    Serial.println("Using static image (not fetching)");
    return;
  }
  
  Serial.println("Fetching image from server...");
  Serial.print("Requesting: https://");
  Serial.print(imageServerAddress);
  Serial.println(imageServerPath);
  
  // Make HTTPS GET request
  client.get(imageServerPath.c_str());
  
  // Read status code
  int statusCode = client.responseStatusCode();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  
  // Print all response headers
  Serial.println("Response headers:");
  while (client.headerAvailable()) {
    String headerName = client.readHeaderName();
    String headerValue = client.readHeaderValue();
    Serial.print("  ");
    Serial.print(headerName);
    Serial.print(": ");
    Serial.println(headerValue);
  }
  Serial.println("--- End of headers ---");
  
  if (statusCode == 200) {
    // Read the response body manually (handles chunked encoding better)
    String base64Image = "";
    Serial.println("Reading response body...");
    
    // Skip headers if not already consumed
    client.skipResponseHeaders();
    
    // Read content length if available
    int contentLength = client.contentLength();
    Serial.print("Content length: ");
    Serial.println(contentLength);
    
    // Read body in chunks
    unsigned long timeout = millis();
    while (client.connected() && (millis() - timeout < 30000)) {
      while (client.available()) {
        char c = client.read();
        base64Image += c;
        timeout = millis(); // Reset timeout on each character received
      }
      
      // Check if we've received all data (for chunked encoding, check if connection closes)
      if (!client.available() && !client.connected()) {
        break;
      }
      
      delay(1); // Small delay to prevent tight loop
    }
    
    Serial.print("Received ");
    Serial.print(base64Image.length());
    Serial.println(" characters");
    
    if (base64Image.length() > 0) {
      // Clear screen before displaying new image
      tft.fillScreen(TFT_BLACK);
      
      // Display the image
      drawJpegFromBase64(base64Image.c_str(), 0, 0);
    } else {
      Serial.println("ERROR: Empty response body!");
    }

  } else {
    Serial.print("HTTP request failed, status code: ");
    Serial.println(statusCode);
    
    // Skip any remaining response body
    client.stop();
  }
}

// Load settings from preferences
void loadSettings() {
  preferences.begin("scryerbook", false);
  
  // Load WiFi settings
  wifiSSID = preferences.getString("wifiSSID", WIFI_SSID);
  wifiPassword = preferences.getString("wifiPassword", WIFI_PASSWORD);
  
  // Load image server settings
  imageServerAddress = preferences.getString("imageServer", "witty-cliff-e5fe8cff64ae48afb25807a538c2dad2.azurewebsites.net");
  imageServerPath = preferences.getString("imagePath", "/?width=240&height=320&rotate=90");
  
  // Load timing settings
  fetchInterval = preferences.getULong("fetchInterval", 20000);
  
  // Load static image setting
  useStaticImage = preferences.getBool("useStatic", false);
  
  preferences.end();
  
  Serial.println("Settings loaded from preferences");
}

// Save settings to preferences
void saveSettings() {
  preferences.begin("scryerbook", false);
  
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPassword", wifiPassword);
  preferences.putString("imageServer", imageServerAddress);
  preferences.putString("imagePath", imageServerPath);
  preferences.putULong("fetchInterval", fetchInterval);
  preferences.putBool("useStatic", useStaticImage);
  
  preferences.end();
  
  Serial.println("Settings saved to preferences");
}

// URL decode helper function
String urlDecode(String str) {
  String decoded = "";
  char c;
  char code0;
  char code1;
  for (unsigned int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%') {
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      decoded += c;
    } else {
      decoded += c;
    }
  }
  return decoded;
}

// Helper for URL decode
unsigned char h2int(char c) {
  if (c >= '0' && c <= '9') {
    return((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return((unsigned char)c - 'A' + 10);
  }
  return(0);
}

// Handle web server requests
void handleWebServer() {
  WiFiClient webClient = webServer.available();
  
  if (webClient) {
    Serial.println("New web client");
    String currentLine = "";
    String request = "";
    bool isPost = false;
    String postData = "";
    
    while (webClient.connected()) {
      if (webClient.available()) {
        char c = webClient.read();
        request += c;
        
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // End of headers, check if POST
            if (isPost && postData.length() == 0) {
              // Read POST data
              while (webClient.available()) {
                postData += (char)webClient.read();
              }
              
              // Parse POST data and update settings
              if (postData.length() > 0) {
                // Parse form data
                int idx;
                
                // WiFi SSID
                idx = postData.indexOf("wifiSSID=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  wifiSSID = urlDecode(postData.substring(idx + 9, endIdx));
                }
                
                // WiFi Password
                idx = postData.indexOf("wifiPassword=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  wifiPassword = urlDecode(postData.substring(idx + 13, endIdx));
                }
                
                // Image Server
                idx = postData.indexOf("imageServer=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  imageServerAddress = urlDecode(postData.substring(idx + 12, endIdx));
                }
                
                // Image Path
                idx = postData.indexOf("imagePath=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  imageServerPath = urlDecode(postData.substring(idx + 10, endIdx));
                }
                
                // Fetch Interval
                idx = postData.indexOf("fetchInterval=");
                if (idx >= 0) {
                  int endIdx = postData.indexOf("&", idx);
                  if (endIdx < 0) endIdx = postData.length();
                  String intervalStr = postData.substring(idx + 14, endIdx);
                  fetchInterval = intervalStr.toInt() * 1000; // Convert seconds to milliseconds
                }
                
                // Static Image checkbox
                useStaticImage = (postData.indexOf("useStatic=on") >= 0);
                
                // Save settings
                saveSettings();
                
                // Send success response
                webClient.println("HTTP/1.1 200 OK");
                webClient.println("Content-Type: text/html");
                webClient.println("Connection: close");
                webClient.println();
                webClient.println("<!DOCTYPE HTML><html><head><meta charset='UTF-8'><title>Settings Saved</title></head>");
                webClient.println("<body><h1>Settings Saved!</h1>");
                webClient.println("<p>Your settings have been saved successfully.</p>");
                webClient.println("<p><a href='/'>Back to Settings</a></p>");
                webClient.println("<p><em>Note: WiFi changes require a restart to take effect.</em></p>");
                webClient.println("</body></html>");
                break;
              }
            }
            
            // Send HTML form (GET request)
            webClient.println("HTTP/1.1 200 OK");
            webClient.println("Content-Type: text/html");
            webClient.println("Connection: close");
            webClient.println();
            
            // HTML form
            webClient.println("<!DOCTYPE HTML><html><head><meta charset='UTF-8'><title>ScryerBook Settings</title>");
            webClient.println("<style>");
            webClient.println("body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; background: #f0f0f0; }");
            webClient.println("h1 { color: #333; }");
            webClient.println("form { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }");
            webClient.println("label { display: block; margin-top: 15px; font-weight: bold; color: #555; }");
            webClient.println("input[type='text'], input[type='password'], input[type='number'] { width: 100%; padding: 8px; margin-top: 5px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }");
            webClient.println("input[type='checkbox'] { margin-top: 10px; }");
            webClient.println("input[type='submit'] { background: #4CAF50; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; margin-top: 20px; font-size: 16px; }");
            webClient.println("input[type='submit']:hover { background: #45a049; }");
            webClient.println(".info { background: #e7f3fe; padding: 10px; border-left: 4px solid #2196F3; margin: 20px 0; }");
            webClient.println("</style></head><body>");
            webClient.println("<h1>ScryerBook Settings</h1>");
            webClient.println("<div class='info'>Static IP: 192.168.1.61</div>");
            webClient.println("<form method='POST' action='/'>");
            
            webClient.println("<label>WiFi SSID:</label>");
            webClient.print("<input type='text' name='wifiSSID' value='");
            webClient.print(wifiSSID);
            webClient.println("'>");
            
            webClient.println("<label>WiFi Password:</label>");
            webClient.print("<input type='password' name='wifiPassword' value='");
            webClient.print(wifiPassword);
            webClient.println("'>");
            
            webClient.println("<label>Image Server Address:</label>");
            webClient.print("<input type='text' name='imageServer' value='");
            webClient.print(imageServerAddress);
            webClient.println("'>");
            
            webClient.println("<label>Image settings:</label>");
            webClient.print("<input type='text' name='imagePath' value='");
            webClient.print(imageServerPath);
            webClient.println("'>");
            
            webClient.println("<label>Fetch Interval (seconds):</label>");
            webClient.print("<input type='number' name='fetchInterval' value='");
            webClient.print(fetchInterval / 1000);
            webClient.println("' min='1'>");
            
            webClient.println("<label>");
            webClient.print("<input type='checkbox' name='useStatic'");
            if (useStaticImage) webClient.print(" checked");
            webClient.println("> Use Static Image (don't fetch from server)</label>");
            
            webClient.println("<input type='submit' value='Save Settings'>");
            webClient.println("</form></body></html>");
            
            break;
          } else {
            // Check for POST request
            if (currentLine.startsWith("POST")) {
              isPost = true;
            }
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    
    delay(1);
    webClient.stop();
    Serial.println("Web client disconnected");
  }
}


