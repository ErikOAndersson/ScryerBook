# ScryingBook Backend

An image processing API built in PHP that downloads, standardizes, resizes, and converts images to base64 format. This tool is designed to fetch images from various sources and process them according to specified dimensions and quality settings.

## Features

- **Image Download**: Fetches images from remote URLs with redirect handling
- **Format Standardization**: Converts images to standardized baseline JPEG format (24-bit RGB)
- **Smart Resizing**: Resizes images with center-crop to maintain aspect ratio
- **Base64 Encoding**: Outputs processed images as base64 strings for easy embedding
- **Multiple Sources**: Supports various image sources including random image generators
- **Debug Logging**: Configurable logging to console and/or page output

## Image Sources

- **Source 0**: Picsum Photos random images (customizable dimensions)
- **Source 1**: Cosmos flower image (fixed URL)
- **Source 2**: Picsum Photos 800x600 random image
- **Source 3**: Picsum Photos specific dog image (ID 237)

## Usage

### Basic Usage

```
https://your-domain.com/index.php
```
Returns a 128x128 base64-encoded image from the default source.

### URL Parameters

| Parameter | Description | Values | Default |
|-----------|-------------|---------|---------|
| `help` | Display help page with usage examples | - | false |
| `info` | Show detailed processing information with visual output | - | false |
| `source` | Select image source | 0-3 | 0 |
| `width` | Desired output width in pixels | 1-2000 | 128 |
| `height` | Desired output height in pixels | 1-2000 | 128 |
| `log` | Logging level | "none", "console", "page", "both" | "none" |

### Example URLs

#### Get help documentation
```
https://your-domain.com/index.php?help
```

#### Basic image processing with custom dimensions
```
https://your-domain.com/index.php?width=256&height=256
```

#### Detailed processing view with specific source
```
https://your-domain.com/index.php?info&source=1&width=300&height=200
```

#### Custom source with logging
```
https://your-domain.com/index.php?source=2&width=400&height=300&log=console
```

#### Square image from random source
```
https://your-domain.com/index.php?source=0&width=512&height=512
```

### Output Modes

1. **Default Mode**: Returns only base64-encoded image data (no HTML)
2. **Info Mode** (`?info`): Returns full HTML page with processing steps, original/standardized/resized images, and base64 output in a textarea

### Response Format

**Default Mode**: Plain text base64 string
```
/9j/4AAQSkZJRgABAQAAAQABAAD/2wBDAAYEBQYFBAYGBQYHBwYIChAKCgkJChQODwwQFxQYGBcU...
```

**Info Mode**: HTML page with detailed processing information and embedded images

## Requirements

- PHP 7.0+
- cURL extension
- GD extension
- Web server (Apache, Nginx, etc.)

## Installation

1. Place `index.php` in your web server's document root
2. Ensure PHP has cURL and GD extensions enabled
3. Access via web browser or HTTP client

## Security Notes

⚠️ **Warning**: This script currently disables SSL certificate verification for development purposes. Enable SSL verification before deploying to production:

```php
curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, true);
curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, 2);
```