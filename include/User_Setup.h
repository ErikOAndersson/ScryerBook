// User_Setup.h for ESP32-C3 with ST7789 display
// Local configuration for TFT_eSPI library
// LOCAL_USER_SETUP_LOADED marker for verification

// Driver selection
#define ST7789_DRIVER

// Display resolution (320x240)
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Color order - swap RED and BLUE
#define TFT_RGB_ORDER TFT_BGR
//#define TFT_RGB_ORDER TFT_RGB

// EA Might be needed?
//#define ST7789_2_INIT  // For some 320x240 displays
//#define CGRAM_OFFSET   // If the display area is offset

// Pin definitions for ESP32-C3
// Avoid GPIO 0, 1 (strapping/USB) and GPIO 11-17 (flash)
#define TFT_MOSI  7   // SDA
#define TFT_SCLK  6   // SCL
#define TFT_CS    10  // Chip select
#define TFT_DC    2   // Data/Command
#define TFT_RST   3   // Reset (set to -1 if not connected)
// #define TFT_BL 4   // Backlight (optional)

// For ESP32-C3, disable touch and use VSPI
#define DISABLE_ALL_LIBRARY_WARNINGS

// Optional: Backlight control
// #define TFT_BL   4
// #define TFT_BACKLIGHT_ON HIGH

// Fonts to load
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.
#define LOAD_GFXFF  // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

#define SMOOTH_FONT

// SPI frequency - reduced for stability on ESP32-C3
#define SPI_FREQUENCY  27000000  // 27MHz (safer than 40MHz)
#define SPI_READ_FREQUENCY  16000000
// #define SPI_TOUCH_FREQUENCY  2500000  // Not used

#define SWAP_BYTES
