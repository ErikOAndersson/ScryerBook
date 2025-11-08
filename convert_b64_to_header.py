#!/usr/bin/env python3
import base64

# Read the Base64 file
with open('src/beach.b64', 'r') as f:
    b64_data = f.read().strip()

# Decode Base64 to bytes
binary_data = base64.b64decode(b64_data)

# Generate C array format
print(f"// Total bytes: {len(binary_data)}")
print("const uint8_t imageData[] PROGMEM = {")

# Format as hex bytes, 16 per line
for i in range(0, len(binary_data), 16):
    chunk = binary_data[i:i+16]
    hex_values = ', '.join(f'0x{b:02x}' for b in chunk)
    print(f"  {hex_values},")

print("};")
print(f"\nconst unsigned int _809_320x240_jpeg_len = {len(binary_data)};")
