import os
from PIL import Image, ImageDraw

COLORS = {
    'folder': (255, 180, 0, 255), # Kora Folder Orange
    'home': (0, 150, 255, 255),   # Blue
    'desktop': (50, 50, 50, 255), # Dark Grey
    'docs': (255, 100, 100, 255), # Red
    'down': (0, 200, 50, 255),    # Green
    'music': (150, 0, 200, 255),  # Purple
    'pic': (255, 0, 150, 255),    # Pink
    'vid': (255, 100, 0, 255)     # Orange
}

OUTPUT_DIR = "kernel/include/icons"
os.makedirs(OUTPUT_DIR, exist_ok=True)

def save_c_header(name, img):
    data = list(img.getdata()) # List of (R,G,B,A)
    filename = f"{OUTPUT_DIR}/kora_{name}.h"
    with open(filename, "w") as f:
        f.write(f"#ifndef KORA_{name.upper()}_H\n")
        f.write(f"#define KORA_{name.upper()}_H\n\n")
        f.write(f"#include <stdint.h>\n\n")
        f.write(f"// 32x32 ARGB Icon: {name}\n")
        f.write(f"static const uint32_t kora_{name}[1024] = {{\n")
        
        for i, p in enumerate(data):
            # ARGB: A<<24 | R<<16 | G<<8 | B
            # Flatten/Premultiply? No, standard blending usually assumes straight or pre-mul.
            # Using 0xAARRGGBB format.
            pixel = (p[3] << 24) | (p[0] << 16) | (p[1] << 8) | p[2]
            f.write(f"0x{pixel:08X}, ")
            if (i+1) % 16 == 0:
                f.write("\n")
        
        f.write("};\n\n")
        f.write(f"#endif\n")

def draw_base_folder(draw, color):
    # Kora style: Rounded rect with tab
    # 32x32 canvas
    # Tab: 2, 4 to 16, 12
    draw.polygon([(2, 6), (14, 6), (16, 10), (30, 10), (30, 28), (2, 28)], fill=color)
    # Highlight/Shadow for depth
    draw.rectangle([4, 12, 28, 26], fill=(255,255,255, 50))

def draw_icon(name, color_key, symbol=None):
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    color = COLORS[color_key]
    
    if name == 'folder':
        draw_base_folder(draw, color)
    elif name == 'file':
        # Paper
        draw.polygon([(6,4), (20,4), (26,10), (26,28), (6,28)], fill=(240,240,240,255))
        # Fold
        draw.polygon([(20,4), (20,10), (26,10)], fill=(200,200,200,255))
    else:
        # Generic circle BG
        draw.ellipse([2,2, 30,30], fill=color)
        # Add letter/symbol
        if symbol:
             # Very basic rendering pixel-by-pixel or simple shapes for symbols
             # Using center rect for now
             if symbol == 'H': # Home
                 draw.polygon([(16,8), (24,16), (24,24), (8,24), (8,16)], fill=(255,255,255,255))
             elif symbol == 'D': # Desktop
                 draw.rectangle([8,8, 24,20], fill=(255,255,255,255))
                 draw.line([16,20, 16,24], fill=(255,255,255,255), width=2)
                 draw.line([12,24, 20,24], fill=(255,255,255,255), width=2)
             elif symbol == 'Down': # Arrow
                 draw.polygon([(12,10), (20,10), (20,18), (24,18), (16,26), (8,18), (12,18)], fill=(255,255,255,255))
             
    save_c_header(name, img)
    print(f"Generated {name}")

def main():
    draw_icon('folder', 'folder')
    draw_icon('file', 'folder') # Reusing folder logic? No.
    # Actually draw_icon needs better logic
    
    # Folders
    draw_icon('folder', 'folder')
    
    # Custom colored folders
    # Not supported by my simple function above well, let's just make specific calls
    
    # 1. Base Folder
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    draw_base_folder(draw, COLORS['folder'])
    save_c_header('folder', img)
    
    # 2. Home (Blue)
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    draw_base_folder(draw, COLORS['home'])
    # House symbol on top
    draw.polygon([(16,14), (24,20), (22,20), (22,25), (10,25), (10,20), (8,20)], fill=(255,255,255,200))
    save_c_header('home', img)
    
    # 3. Desktop (Grey)
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    draw_base_folder(draw, COLORS['desktop'])
    save_c_header('desktop', img)
    
    # 4. Downloads (Green)
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    draw_base_folder(draw, COLORS['down'])
    # Arrow
    draw.polygon([(16,24), (22,18), (10,18)], fill=(255,255,255,200))
    draw.rectangle([14,12, 18,18], fill=(255,255,255,200))
    save_c_header('downloads', img)
    
    # 5. Documents (Red)
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    draw_base_folder(draw, COLORS['docs'])
    save_c_header('documents', img)
    
    # 6. Pictures (Pink)
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    draw_base_folder(draw, COLORS['pic'])
    save_c_header('pictures', img)
    
    # 7. Music (Purple)
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    draw_base_folder(draw, COLORS['music'])
    save_c_header('music', img)
    
    # 8. Videos (Orange)
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    draw_base_folder(draw, COLORS['vid'])
    save_c_header('videos', img)

    # 9. File (Generic)
    img = Image.new('RGBA', (32, 32), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    draw.polygon([(6,4), (20,4), (26,10), (26,28), (6,28)], fill=(240,240,240,255))
    draw.polygon([(20,4), (20,10), (26,10)], fill=(200,200,200,255))
    # Text lines
    for y in range(14, 26, 4):
        draw.line([10, y, 22, y], fill=(180,180,180,255), width=1)
    save_c_header('file', img)

if __name__ == "__main__":
    main()
