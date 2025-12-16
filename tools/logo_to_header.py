
import sys
from PIL import Image

def convert_logo(input_path, output_path):
    try:
        img = Image.open(input_path)
        img = img.resize((1024, 768))   # Resize to screen resolution
        img = img.convert("RGB")        # Flatten to RGB
        
        with open(output_path, 'w') as f:
            f.write("#ifndef BOOTLOGO_H\n")
            f.write("#define BOOTLOGO_H\n\n")
            f.write("#include <stdint.h>\n\n")
            f.write("static const uint32_t bootlogo_data[1024 * 768] = {\n")
            
            data = list(img.getdata())
            count = 0
            for r, g, b in data:
                # 0xFFRRGGBB (ARGB)
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
        print("Usage: python3 logo_to_header.py <input_img> <output_header>")
        sys.exit(1)
        
    convert_logo(sys.argv[1], sys.argv[2])
