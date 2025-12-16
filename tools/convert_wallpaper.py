import os
from PIL import Image

def convert():
    src = "wallpapers/converted.png"
    dst = "kernel/include/wallpaper_data.h"
    
    if not os.path.exists(src):
        print(f"Error: {src} not found")
        return

    print("Loading image...")
    img = Image.open(src)
    img = img.resize((1024, 768))
    img = img.convert("RGBA")
    
    print("Writing header...")
    with open(dst, "w") as f:
        f.write("#ifndef WALLPAPER_DATA_H\n")
        f.write("#define WALLPAPER_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write("// generated from wallpaper.png\n")
        f.write("static const uint32_t wallpaper_data[1024 * 768] = {\n")
        
        data = list(img.getdata())
        
        # Buffer writes
        chunk_size = 1024
        for i in range(0, len(data), chunk_size):
            chunk = data[i:i+chunk_size]
            lines = []
            for r, g, b, a in chunk:
                # 0xAARRGGBB format generally expected
                val = (a << 24) | (r << 16) | (g << 8) | b
                lines.append(f"0x{val:08X}")
            f.write(",".join(lines) + ",")
            f.write("\n")
            
        f.write("};\n\n")
        f.write("#endif\n")
        
    print("Done.")

if __name__ == "__main__":
    convert()
