
import os
from PIL import Image, ImageFont, ImageDraw

def generate_font_header():
    # Configuration
    font_path = "letterfonts/fonts/San Francisco/SF-Pro-Text-Regular.otf"
    output_path = "kernel/include/font_sf.h"
    font_size = 13 # Adjust for roughly 16px line height or visual appeal
    # Note: explicit height of 16px in array means we need to fit char in 16px vertical.
    # 13pt usually fits well within 16px.
    
    array_height = 16
    
    if not os.path.exists(font_path):
        print(f"Error: Font not found at {font_path}")
        return

    try:
        font = ImageFont.truetype(font_path, font_size)
    except Exception as e:
        print(f"Error loading font: {e}")
        return

    print(f"Generating {output_path}...")
    
    with open(output_path, "w") as f:
        f.write("#ifndef FONT_SF_H\n")
        f.write("#define FONT_SF_H\n\n")
        f.write("#include <stdint.h>\n\n")
        
        f.write(f"#define SF_HEIGHT {array_height}\n\n")
        
        # We will generate for ASCII 32 - 126
        # Data structure:
        # uint8_t font_sf_widths[128]; // Store width for each char
        # uint16_t font_sf_data[128][16]; // Store 16 rows of up to 16 pixels width
        
        widths = []
        bitmaps = []
        
        # Prepare arrays
        for i in range(128):
            widths.append(0)
            bitmaps.append([0] * array_height)
            
        print("Rendering characters...")
        
        for char_code in range(32, 127):
            char = chr(char_code)
            
            # Get bounding box
            # getbbox returns (left, top, right, bottom)
            bbox = font.getbbox(char)
            if bbox:
                w = bbox[2] - bbox[0]
                h = bbox[3] - bbox[1]
            else:
                # Space or empty
                w = 0 
                # For space, we might want a default width
                if char == ' ':
                    # use getlength for advance width
                    w = int(font.getlength(char))
                
            # Clamp width to 16 for uint16 storage
            if w > 16: w = 16
            
            widths[char_code] = w
            
            # Create image to draw
            # We use a canvas of 16x16
            img = Image.new("1", (16, array_height), 0)
            draw = ImageDraw.Draw(img)
            
            # Centering vertically? Or baseline?
            # Standard fonts have baseline. 
            # Let's try to draw at fixed position to align baselines.
            # Usually strict top-left or offset.
            # font.getmetrics() -> (ascent, descent)
            ascent, descent = font.getmetrics()
            
            # We want baseline to be constant. 
            # If array_height is 16, and font_size 13.
            # Let's place baseline at row 12 or 13.
            # draw.text((x, y)) draws top-left by default.
            # But we can use anchor too.
            # Simple approach: draw at (0, 0) and see where it lands relative to ascent.
            
            # Calculate Y offset to center roughly
            # total_height = ascent + descent
            # y_offset = (array_height - total_height) // 2
            # But we need baseline alignment.
            # Let's assume y=1 matches top of ascent?
            # actually draw.text pos depends on library version default. PIL usually top-left of bbox or ascender.
            
            # Let's align ascent to row 2 (leaving 2 pixel top margin potentially)
            draw.text((0, 0), char, font=font, fill=1)
            
            # Capture data
            pixels = list(img.getdata()) # flattened 256 items
            
            # Convert to bitmask rows
            char_data = []
            for r in range(array_height):
                row_val = 0
                for c in range(16): # 0..15
                    if pixels[r * 16 + c]:
                        row_val |= (1 << (15 - c)) # MSB left
                char_data.append(row_val)
            
            bitmaps[char_code] = char_data
            
        # Write widths
        f.write("static const uint8_t font_sf_widths[128] = {\n")
        for i in range(0, 128, 16):
            chunk = [str(x) for x in widths[i:i+16]]
            f.write("    " + ", ".join(chunk) + ",\n")
        f.write("};\n\n")
        
        # Write bitmaps
        f.write("static const uint16_t font_sf_data[128][16] = {\n")
        for i in range(128):
            f.write(f"    // Char {i} '{chr(i) if 32<=i<=126 else '.'}'\n")
            f.write("    {")
            row_strs = [f"0x{x:04X}" for x in bitmaps[i]]
            f.write(", ".join(row_strs))
            f.write("},\n")
        f.write("};\n\n")
        
        f.write("#endif // FONT_SF_H\n")
        
    print("Done.")

if __name__ == "__main__":
    generate_font_header()
