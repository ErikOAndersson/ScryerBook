#include <TFT_eSPI.h>                                            
#include "beach.h"

void drawSpots();
void Blip(int blips);


TFT_eSPI tft = TFT_eSPI();  

 // do not forget User_Setup_xxxx.h matching microcontroller-display combination
 // my own: <Setup_FW_ESP32C3DEV_ST7789.h>
  
#define SCOPE   0x3206                             // CRT scope glass color - CRT like luminescent 
#define PIN_LED 21


// some other principal colors
// RGB 565 color picker at https://ee-programming-notepad.blogspot.com/2016/10/16-bit-color-generator-picker.html
// #define WHITE       0xFFFF
// #define BLACK       0x0000
// #define BLUE        0x001F
// #define RED         0xF800
// #define GREEN       0x07E0
// #define CYAN        0x07FF
// #define MAGENTA     0xF81F
// #define YELLOW      0xFFE0
// #define GREY        0x2108 
// #define ORANGE      0xFBE0
// #define TEXT_COLOR  0xFFFF                         // is currently white 
// #define AFRICA      0xAB21
// #define BORDEAUX    0xA000
// #define VOLTMETER   0xF6D3     

int delayTime = 500;
int j,t;
int count = 0;

// colors are:       bordeaux-africa - green - blue  - red  - yellow -bordeaux- africa - green - blue  - red  - yellow -bordeaux in hex, see defines above
int color [13] = {  0xA000, 0xAB21, 0x07E0, 0x001F, 0xF800, 0xFFE0,  0xA000,  0xAB21,  0x07E0, 0x001F, 0xF800, 0xFFE0, 0xA000};


void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  Blip(10);

  Serial.begin (9600);
  delay(1000);  // Give serial time to initialize

  Serial.println("Initializing TFT...");
  Serial.print("TFT_CS: "); Serial.println(TFT_CS);
  Serial.print("TFT_DC: "); Serial.println(TFT_DC);
  Serial.print("TFT_MOSI: "); Serial.println(TFT_MOSI);
  Serial.print("TFT_SCLK: "); Serial.println(TFT_SCLK);
  Serial.print("TFT_RST: "); Serial.println(TFT_RST);
  
  delay(100);  // Let pins stabilize
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
  tft.invertDisplay(true);
  j=12; 

  Serial.println ("Starting up...");
}

void loop() {
  Serial.print(count);
  Serial.print(", ");
  //Blip(1);

  // Draw a line diagonally across the screen
   tft.drawLine(0, 0, tft.width(), tft.height(), TFT_GREEN);


  count++;
  if (count > 9999){
    count = 0;
  } 

  drawSpots();

  delay(5000);

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
    Serial.print(".");
  }
}