# ScryerBook

A multi-function ESP32-C3 display device with analog clock, dynamic image fetching, and quote display capabilities.

## Overview

ScryerBook is an embedded application for ESP32-C3 microcontrollers with GC9A01 round TFT displays. The device operates in multiple modes, fetching content over WiFi and rendering it on a circular display with hardware sprite rotation.

## Features

### Operating Modes

- **Clock Mode**: Analog clock face with Roman numerals, rendered using custom font and background image from LittleFS
- **Image Fetch Mode**: Retrieves Base64-encoded JPEG images from Azure Web App via HTTPS, decodes and renders in real-time
- **Quote Mode**: Fetches inspirational quotes from API Ninjas REST API with automatic fallback to cached content
- **Status Info Mode**: Displays system diagnostics including network profile, IP configuration, DNS servers, WiFi signal strength, memory usage, and uptime
- **Network Picker Mode**: Interactive WiFi profile selector with on-screen configuration display

### Network Management

- Multiple network profile support with automatic fallback
- Static IP configuration with custom DNS server assignment
- DNS configuration persistence across network reconnections
- Per-profile WiFi credentials (SSID/password)
- Runtime network profile switching via hardware button

### Web Interface

- HTTP server for remote configuration (port 80)
- Settings management: image fetch interval, status update interval, server configuration
- Persistent storage using ESP32 Preferences API

## Hardware

- **Microcontroller**: ESP32-C3 Super Mini
- **Display**: GC9A01 240x240 round TFT (configured for 320x240 with 90° rotation)
- **Storage**: LittleFS filesystem on flash for background images
- **Input**: Two GPIO buttons (pins 8, 9)
- **Output**: Status LED (pin 21)

## Software Architecture

### Core Modules

- **main.cpp/h**: Application entry point, mode switching, display rendering, settings management
- **network.cpp/h**: WiFi connection management, network profile handling, DNS configuration
- **fetchimage.cpp/h**: HTTPS client for Azure image server, Base64 decoder, JPEG rendering pipeline
- **quote.cpp/h**: API Ninjas integration, JSON parsing, quote retrieval
- **clock.cpp/h**: Analog clock rendering with sprite-based background compositing
- **webserver.cpp/h**: HTTP server for web-based configuration interface
- **settings.cpp/h**: Preferences API wrapper for persistent configuration storage

### Technical Details

**Display Pipeline**
- Full-screen sprite buffer (320x240, 16-bit color depth)
- Hardware-accelerated 90° rotation via `pushRotated()`
- Efficient line-by-line RGB565 image loading from LittleFS
- JPEG decode and render using JPEGDecoder library

**Network Stack**
- WiFiClientSecure with certificate validation bypass for development
- Custom DNS server configuration (Google DNS, Cloudflare, corporate DNS)
- DNS persistence through reconnection via dual `WiFi.config()` calls
- Chunked transfer encoding support for HTTP responses

**Memory Management**
- Dynamic buffer allocation for Base64/JPEG decode operations
- Sprite deletion during image fetch to free ~150KB RAM
- Heap monitoring with ESP.getFreeHeap() / ESP.getMinFreeHeap()

**Image Server Protocol**
- HTTPS connection to Azure Web App
- Query parameters: `?width=240&height=320&rotate=270`
- Base64-encoded JPEG response with chunked transfer encoding
- Configurable buffer size with automatic truncation

## Dependencies

- **TFT_eSPI**: Display driver (Bodmer)
- **JPEGDecoder**: JPEG decompression (Bodmer)
- **ArduinoJson**: JSON parsing v7.2.0+ (Benoit Blanchon)
- **ArduinoHttpClient**: HTTP client library
- **LittleFS**: Flash filesystem
- **WiFiClientSecure**: TLS/SSL support

## Build Configuration

**Platform**: ESP32 (espressif32@6.5.0)  
**Framework**: Arduino  
**Board**: ESP32-C3-DevKitM-1  
**Filesystem**: LittleFS with custom partition table  
**Monitor Speed**: 115200 baud

Build flags enable USB CDC and include custom TFT_eSPI configuration from `include/User_Setup.h`.

## Configuration

### WiFi Credentials Setup

1. Copy `include/wifi_credentials.h.example` to `include/wifi_credentials.h`
2. Edit `wifi_credentials.h` with your actual WiFi SSID, password, and API key
3. The credentials file is excluded from git to protect sensitive information

### Network Profiles

Network profiles with IP/DNS configuration are defined in `src/network.cpp`.

Settings are stored in ESP32 NVS flash and persisted across reboots.

## License

Project files are provided as-is for educational and development purposes.
