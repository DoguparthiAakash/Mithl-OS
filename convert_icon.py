from PIL import Image
import sys

def convert_icon(path):
    try:
        img = Image.open(path)
        img = img.resize((48, 48), Image.Resampling.LANCZOS)
        img = img.convert("RGBA")
        
        pixels = list(img.getdata())
        
        print("static const uint32_t icon_app_doom[48 * 48] = {")
        
        for i, p in enumerate(pixels):
            r, g, b, a = p
            # Format: 0xAARRGGBB
            val = (a << 24) | (r << 16) | (g << 8) | b
            sys.stdout.write(f"0x{val:08X},")
            if (i + 1) % 16 == 0:
                sys.stdout.write("\n")
                
        print("};")
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 convert_icon.py <image_path> <c_array_name>")
        sys.exit(1)
    
    path = sys.argv[1]
    name = sys.argv[2]
    
    try:
        img = Image.open(path)
        img = img.resize((32, 32), Image.Resampling.LANCZOS) # Resize to 32x32 for UI
        img = img.convert("RGBA")
        
        pixels = list(img.getdata())
        
        print(f"static const uint32_t {name}[32 * 32] = {{")
        
        for i, p in enumerate(pixels):
            r, g, b, a = p
            # Format: 0xAARRGGBB
            val = (a << 24) | (r << 16) | (g << 8) | b
            sys.stdout.write(f"0x{val:08X},")
            if (i + 1) % 16 == 0:
                sys.stdout.write("\n")
                
        print("};")
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
