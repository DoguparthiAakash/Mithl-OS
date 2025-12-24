import struct

width = 1024
height = 768
filename = "wallpaper.bmp"

def create_gradient_bmp(width, height, filename):
    # BMP Header
    file_size = 14 + 40 + (width * height * 4)
    offset = 54
    
    # 14 bytes file header
    # B M, Size, Res, Res, Offset
    header = struct.pack('<2sIHHI', b'BM', file_size, 0, 0, offset)
    
    # 40 bytes info header
    # Size, W, H, Planes, BPP, Compression, ImageSize, Xppm, Yppm, Colors, ImpColors
    info_header = struct.pack('<iiiHHIIiiII', 40, width, height, 1, 32, 0, width*height*4, 2835, 2835, 0, 0)
    
    with open(filename, 'wb') as f:
        f.write(header)
        f.write(info_header)
        
        # Pixels (Bottom-Up)
        for y in range(height):
            for x in range(width):
                # Gradient: Blue to Purple
                r = int(x / width * 255)
                g = int(y / height * 128)
                b = 200
                a = 255
                # BGRA for BMP
                f.write(struct.pack('BBBB', b, g, r, a))

    print(f"Generated {filename}")

if __name__ == "__main__":
    create_gradient_bmp(width, height, filename)
