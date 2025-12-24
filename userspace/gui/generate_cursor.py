import struct

# 12x19 Arrow Cursor
# Key: . = Transparent, X = Black, W = White
# Simple classic arrow
cursor_data = [
    "X           ",
    "XX          ",
    "XWX         ",
    "XWWX        ",
    "XWWWX       ",
    "XXXXXX      ",
    "XWXX        ",
    "XX          ",
    "            ",
    "            ",
    "            ",
    "            ",
]

# We'll make it 16x16 for simplicity or stick to the shape
width = 12
height = 12
filename = "cursor.bmp"

def create_cursor_bmp():
    # BMP Header
    file_size = 14 + 40 + (width * height * 4)
    offset = 54
    header = struct.pack('<2sIHHI', b'BM', file_size, 0, 0, offset)
    info_header = struct.pack('<iiiHHIIiiII', 40, width, height, 1, 32, 0, width*height*4, 2835, 2835, 0, 0)
    
    with open(filename, 'wb') as f:
        f.write(header)
        f.write(info_header)
        
        # Pixels (Bottom-Up)
        # Our data is Top-Down, so reverse
        for y in range(height - 1, -1, -1):
            row_str = cursor_data[y]
            for x in range(width):
                char = row_str[x]
                if char == 'X':
                    # Black
                    f.write(struct.pack('BBBB', 0, 0, 0, 255))
                elif char == 'W':
                    # White
                    f.write(struct.pack('BBBB', 255, 255, 255, 255))
                else:
                    # Transparent
                    f.write(struct.pack('BBBB', 0, 0, 0, 0)) # Alpha 0
                    
    print(f"Generated {filename}")

if __name__ == "__main__":
    create_cursor_bmp()
