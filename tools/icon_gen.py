
import sys
import struct

# Simple hardcoded 48x48 icon generators to match "Kora" aesthetic (Flat, colorful)

def create_computer_icon():
    # 48x48 RGBA
    pixels = [0] * (48 * 48)
    
    # Monitor Frame (Dark Grey)
    for y in range(4, 34):
        for x in range(4, 44):
            if x < 6 or x > 41 or y < 6 or y > 31: # Border
                pixels[y * 48 + x] = 0xFF404552 # Dark Blue-Grey
            else: # Screen
                pixels[y * 48 + x] = 0xFF282C34 # Darker Screen
                
    # Screen Glare (Diagonal)
    for i in range(20):
        if 6+i < 32 and 30-i > 4:
            pixels[(6+i) * 48 + (30-i)] = 0xFF3E4451

    # Stand
    for y in range(34, 40):
        for x in range(20, 28):
            pixels[y * 48 + x] = 0xFF9DA5B4
            
    # Base
    for y in range(40, 44):
        for x in range(16, 32):
            pixels[y * 48 + x] = 0xFFABACAE

    return pixels

def create_folder_icon():
    pixels = [0] * (48 * 48)
    
    # Back flap (Darker)
    for y in range(6, 12):
        for x in range(4, 20):
            pixels[y * 48 + x] = 0xFFE0AE4B # Gold/Yellow darken
            
    # Main Body
    for y in range(10, 42):
        for x in range(4, 44):
            pixels[y * 48 + x] = 0xFFF0C674 # Gold/Yellow
            
    # Shadow/Detail
    for y in range(10, 12): # Top crease
        for x in range(4, 44):
            pixels[y * 48 + x] = 0xFFFFD785
            
    return pixels

def create_terminal_icon():
    pixels = [0] * (48 * 48)
    
    # Background (Dark Grey/Black)
    for y in range(6, 42):
        for x in range(4, 44):
            pixels[y * 48 + x] = 0xFF282C34
    
    # Top Bar
    for y in range(6, 12):
        for x in range(4, 44):
            pixels[y * 48 + x] = 0xFF3E4451
            
    # Prompt >
    # Line 1 (Green)
    for i in range(3): # > shape
        pixels[(16+i)*48 + (10+i)] = 0xFF98C379
        pixels[(20-i)*48 + (10+i)] = 0xFF98C379
        
    # Cursor (White block)
    for y in range(16, 21):
        for x in range(16, 20):
            pixels[y * 48 + x] = 0xFFABB2BF

    return pixels

def create_file_icon():
    pixels = [0] * (48 * 48)
    
    # Paper Body (White)
    for y in range(4, 44):
        for x in range(8, 40):
            pixels[y * 48 + x] = 0xFFF0F0F0
            
    # Dog-ear (Top Right)
    for y in range(4, 12):
        for x in range(32, 40):
            if x > 32 + (y-4):
                pixels[y * 48 + x] = 0 # Cut
            elif x == 32 + (y-4):
                pixels[y * 48 + x] = 0xFFCCCCCC # Fold shadow

    # Text Lines
    for line in range(4):
        y_base = 16 + line * 6
        for x in range(12, 36):
             pixels[y_base * 48 + x] = 0xFF999999
             
    return pixels

def write_c_header(filename):
    computer = create_computer_icon()
    folder = create_folder_icon()
    terminal = create_terminal_icon()
    
    with open(filename, 'w') as f:
        f.write("#ifndef ICONS_DATA_H\n")
        f.write("#define ICONS_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        
        f.write("#define ICON_WIDTH 48\n")
        f.write("#define ICON_HEIGHT 48\n\n")
        
        def write_array(name, data):
            f.write(f"static const uint32_t {name}[] = {{\n")
            c = 0
            for p in data:
                f.write(f"0x{p:08X}, ")
                c += 1
                if c % 8 == 0: f.write("\n")
            f.write("};\n\n")
            
        write_array("icon_computer", computer)
        write_array("icon_folder", folder)
        write_array("icon_terminal", terminal)
        write_array("icon_file", create_file_icon())
        
        f.write("#endif\n")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: icon_gen.py <output_header>")
    else:
        write_c_header(sys.argv[1])
        print(f"Generated {sys.argv[1]}")
