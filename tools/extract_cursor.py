import struct
import sys

def parse_xcursor(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()

    if data[:4] != b'Xcur':
        print("Not an XCursor file")
        return

    header_size, version, ntoc = struct.unpack('<III', data[4:16])
    print(f"Header: ver={version}, ntoc={ntoc}")

    # Parse TOC
    toc_start = 16
    for i in range(ntoc):
        # Type (4), Subtype (4), Position (4)
        entry_offset = toc_start + (i * 12)
        type_, subtype, pos = struct.unpack('<III', data[entry_offset:entry_offset+12])
        
        # Type 0xfffd0002 is Image
        if type_ == 0xfffd0002:
            # Parse Image Header at pos
            # Header: Size(4), Type(4), Subtype(4), Version(4), Width(4), Height(4), Xhot(4), Yhot(4), Delay(4)
            img_header = data[pos:pos+36]
            size, type2, subtype2, ver, width, height, xhot, yhot, delay = struct.unpack('<IIIIIIIII', img_header)
            
            print(f"Image found: {width}x{height} at pos {pos}")
            
            if width == 32 and height == 32:
                # Found a 32x32 cursor
                pixel_start = pos + 36
                pixels = data[pixel_start:pixel_start + (width * height * 4)]
                return width, height, xhot, yhot, pixels

    return None

def main():
    if len(sys.argv) < 3:
        print("Usage: extract_cursor.py <input_xcursor> <output_header>")
        sys.exit(1)

    result = parse_xcursor(sys.argv[1])
    if not result:
        print("No suitable 32x32 image found")
        sys.exit(1)

    width, height, xhot, yhot, pixels = result
    
    with open(sys.argv[2], 'w') as out:
        out.write("#ifndef CURSOR_ICON_H\n#define CURSOR_ICON_H\n\n")
        out.write(f"#define CURSOR_WIDTH {width}\n")
        out.write(f"#define CURSOR_HEIGHT {height}\n")
        out.write(f"#define CURSOR_HOT_X {xhot}\n")
        out.write(f"#define CURSOR_HOT_Y {yhot}\n\n")
        out.write("static const uint32_t cursor_icon[] = {\n")
        
        # Write pixels (ARGB)
        # Assuming Little Endian (BGRA or RGBA depending on XCursor spec, usually ARGB)
        # Xcursor is ARGB (0xAARRGGBB)
        
        count = 0
        for i in range(0, len(pixels), 4):
            # Read as uint32
            pixel = struct.unpack('<I', pixels[i:i+4])[0]
            out.write(f"0x{pixel:08x}, ")
            count += 1
            if count % 8 == 0:
                out.write("\n")
        
        out.write("};\n\n#endif\n")
    print(f"Generated {sys.argv[2]}")

if __name__ == "__main__":
    main()
