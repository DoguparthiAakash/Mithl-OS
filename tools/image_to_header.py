
import sys
from PIL import Image

def convert_image(input_path, output_path):
    try:
        # Load image
        img = Image.open(input_path)
        img = img.resize((1024, 768)) # Resize to screen resolution
        img = img.convert("RGB") # Flatten to RGB (no alpha needed for wallpaper)
        
        # We will export as 32-bit ARGB 0xFFRRGGBB for simplicity with existing drawing
        # But to save space/time, maybe 24bit? 
        # C array: static const uint32_t wallpaper[1024*768] is 3MB source code. Huge compile time.
        # GCC might choke on 3MB array in header.
        # Alternative: Load raw binary file? No filesystem support for reading huge files yet?
        # Check filesystem.c -> It has ramdisk?
        # RAMDISK is best. But assuming I can't easily add to ramdisk (initrd).
        # I'll try generating the header. usage: static const uint32_t wallpaper[] ...
        
        # Optimization: RLE? No, decoder needed.
        # Let's try the header. If it fails, we fall back to solid color.
        
        with open(output_path, 'w') as f:
            f.write("#ifndef WALLPAPER_DATA_H\n")
            f.write("#define WALLPAPER_DATA_H\n\n")
            f.write("#include <stdint.h>\n\n")
            f.write("#define WALLPAPER_WIDTH 1024\n")
            f.write("#define WALLPAPER_HEIGHT 768\n\n")
            f.write("// WARNING: This file is huge.\n")
            f.write("static const uint32_t wallpaper_data[] = {\n")
            
            data = list(img.getdata())
            count = 0
            for r, g, b in data:
                # 0xFFRRGGBB
                pixel = (0xFF << 24) | (r << 16) | (g << 8) | b
                f.write(f"0x{pixel:08X},")
                count += 1
                if count % 16 == 0:
                    f.write("\n")
            
            f.write("};\n\n")
            f.write("#endif\n")
            
        print(f"Converted {input_path} to {output_path}")

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 image_to_header.py <input_img> <output_header>")
        sys.exit(1)
        
    convert_image(sys.argv[1], sys.argv[2])
