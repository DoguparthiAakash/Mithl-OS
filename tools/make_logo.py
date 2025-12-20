
import sys
import os
from PIL import Image

def main():
    if len(sys.argv) < 3:
        print("Usage: python3 make_logo.py <input.png> <output.h>")
        return

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    try:
        img = Image.open(input_path)
        img = img.convert("RGBA")
        
        # Resize if too large (e.g. keep max dimension 256 for icon style)
        # MacOS boot apple is roughly 10-15% of screen height.
        # Let's target ~192x192 or similar. 
        # If user wants "same size as apple of macos", that's roughly 150-200px on a standard display.
        max_size = (192, 192)
        img.thumbnail(max_size, Image.Resampling.LANCZOS)
        
        width, height = img.size
        pixels = list(img.getdata())
        
        with open(output_path, "w") as f:
            f.write("#ifndef BOOTLOGO_H\n")
            f.write("#define BOOTLOGO_H\n\n")
            f.write("#include \"types.h\"\n\n")
            f.write(f"static const int bootlogo_width = {width};\n")
            f.write(f"static const int bootlogo_height = {height};\n\n")
            f.write(f"static const uint32_t bootlogo_data[{width * height}] = {{\n")
            
            for i, p in enumerate(pixels):
                # PIL RGBA -> 0xAARRGGBB
                r, g, b, a = p
                # Check graphics.c expectation. 
                # write_pixel_32: alpha = src >> 24. r = src >> 16.
                # So expected format is 0xAARRGGBB.
                val = (a << 24) | (r << 16) | (g << 8) | b
                f.write(f"0x{val:08X},")
                if (i + 1) % 16 == 0:
                    f.write("\n")
            
            f.write("\n};\n\n")
            f.write("#endif\n")
            
        print(f"Generated {output_path} with size {width}x{height}")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
