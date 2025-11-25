#include "clock.h"
#include "main.h"
#include "font_gilligan_10.h"
#include <LittleFS.h>
#include <math.h>

// Time information structure
struct tm timeinfo;

// External references to TFT and sprite from main.cpp
extern TFT_eSPI tft;
extern TFT_eSprite sprite;

// External reference to image loading function
extern void loadImageToSpriteFromLittleFS(TFT_eSprite &spr, const char* filename, int xpos, int ypos, int imageWidth, int imageHeight, uint16_t transparentColor);

// Initialize clock system
void initClock() {
  // Nothing to initialize currently
}

void DrawClockFace() {
  DrawClockFace(CLOCK_WIDTH / 2, CLOCK_HEIGHT / 2, (CLOCK_WIDTH / 2) * CLOCK_SCALE);
}

void DrawClockFace(int xpos, int ypos, float scale) {
  // Draw clock face on sprite (sprite should already be cleared/filled by caller)
  sprite.setTextColor(TFT_WHITE); // White text
  const float indlen = 0.9; // Indicator length factor
  // Scale text size proportionally
  //sprite.setTextSize(max(1, (int)(2 * CLOCK_SCALE)));
  // Use fixed size instead
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
  //loadImageToSpriteFromLittleFS(sprite, "/backclock.rgb565", 0, 0, SPRITE_WIDTH, SPRITE_HEIGHT, 0xFFFF);
  // Trying out .jpg instead
  loadJpegToSpriteFromLittleFS(sprite, "/backclock.jpg", 0, 0);
  
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
