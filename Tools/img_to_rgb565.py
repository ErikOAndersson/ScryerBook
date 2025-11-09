# Python script to convert image to RGB565 format
from PIL import Image
import os
import sys
import re

# Check command line arguments
if len(sys.argv) < 2:
    print("Usage: python img_to_rgb565.py <image_file>")
    print("Example: python img_to_rgb565.py WatchFace_1.jpg")
    sys.exit(1)

# Input image file from command line
input_file = sys.argv[1]

# Check if file exists
if not os.path.exists(input_file):
    print(f"Error: File '{input_file}' not found!")
    sys.exit(1)

# Generate output filename and array name from input filename
# Remove extension and path, replace non-alphanumeric with underscore
base_name = os.path.splitext(os.path.basename(input_file))[0]
# Convert to valid C identifier (replace invalid chars with underscore)
array_name = re.sub(r'[^a-zA-Z0-9_]', '_', base_name) + '_img'
output_file = base_name + '.h'

print(f"Converting {input_file} to RGB565...")

# Open and resize image
img = Image.open(input_file).convert('RGB').resize((320, 240))
print(f"Image size: {img.size}")
print(f"Image mode: {img.mode}")

# Test several pixels to verify the image has data
print("\nSampling pixels:")
test_coords = [(0, 0), (160, 120), (319, 239), (100, 100), (200, 150)]
for x, y in test_coords:
    r, g, b = img.getpixel((x, y))
    print(f"  Pixel ({x:3d}, {y:3d}): RGB({r:3d}, {g:3d}, {b:3d})")

# Check if image is all black
pixels = list(img.getdata())
non_black = sum(1 for p in pixels if p != (0, 0, 0))
print(f"\nTotal pixels: {len(pixels)}")
print(f"Non-black pixels: {non_black}")
print(f"Black pixels: {len(pixels) - non_black}")

if non_black == 0:
    print("\nWARNING: Image appears to be completely black!")
    print("Check if the source image file is correct.")
else:
    print(f"\nImage looks good! {non_black/len(pixels)*100:.1f}% non-black pixels")

with open(output_file, 'w') as f:
    f.write(f'#ifndef {array_name.upper()}_H\n')
    f.write(f'#define {array_name.upper()}_H\n\n')
    f.write(f'// Image size: 320x240 pixels\n')
    f.write(f'// Format: RGB565 (16-bit color)\n')
    f.write(f'const uint16_t {array_name}[76800] PROGMEM = {{\n')
    
    pixel_count = 0
    for y in range(240):
        f.write('  ')  # Indentation
        for x in range(320):
            r, g, b = img.getpixel((x, y))
            # Convert RGB888 to RGB565
            # R: 8 bits -> 5 bits (keep top 5)
            # G: 8 bits -> 6 bits (keep top 6) 
            # B: 8 bits -> 5 bits (keep top 5)
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            
            f.write(f'0x{rgb565:04X}')
            pixel_count += 1
            
            if pixel_count < 76800:
                f.write(',')
            
            # Add newline every 16 pixels for readability
            if x < 319 and (x + 1) % 16 == 0:
                f.write('\n  ')
        f.write('\n')
    
    f.write('};\n\n')
    f.write(f'#endif // {array_name.upper()}_H\n')

# Add after the normal .h file generation:
with open(base_name + '.rgb565', 'wb') as f:
    for y in range(240):
        for x in range(320):
            r, g, b = img.getpixel((x, y))
            rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
            # Write as little-endian uint16
            f.write(rgb565.to_bytes(2, byteorder='little'))

print(f"Conversion complete! Output: {output_file}")
print(f"Total pixels: {pixel_count}")
print(f"File size: ~{os.path.getsize(output_file) / 1024:.1f} KB (source)")
print(f"Array size: {320 * 240 * 2} bytes ({320 * 240 * 2 / 1024:.1f} KB in flash)")


