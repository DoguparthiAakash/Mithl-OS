import struct
import sys
from PIL import Image

def extract_cursor(xcursor_path, output_bmp):
    try:
        with open(xcursor_path, 'rb') as f:
            # XCursor Header: Magic (4) + Header (4) + Version (4) + NTOC (4)
            magic = f.read(4)
            if magic != b'Xcur':
                print("Not a valid XCursor file")
                return

            header_len = struct.unpack('<I', f.read(4))[0]
            version = struct.unpack('<I', f.read(4))[0]
            ntoc = struct.unpack('<I', f.read(4))[0]
            
            print(f"XCursor: Ver {version}, Chunks {ntoc}")
            
            # Read TOC
            toc_items = []
            for _ in range(ntoc):
                # Type (4), Subtype (4), Position (4)
                type_, subtype, pos = struct.unpack('<III', f.read(12))
                toc_items.append((type_, subtype, pos))
                
            # Find the best image (Type 0xfffd0002 is Image)
            # We want a reasonable size, say 32x32 or 24x24
            best_chunk_pos = 0
            best_size = 0
            
            for type_, subtype, pos in toc_items:
                if type_ == 0xfffd0002: # IMAGE TYPE
                    # Jump to position to read header
                    cur_pos = f.tell()
                    f.seek(pos)
                    # Image Header: HeaderSize(4), Type(4), Subtype(4), Version(4), Width(4), Height(4), Xhot(4), Yhot(4), Delay(4)
                    chunk_header = struct.unpack('<IIIIIIIII', f.read(36))
                    width = chunk_header[4]
                    height = chunk_header[5]
                    
                    print(f"Found Image: {width}x{height}")
                    
                    # Prefer 32px or closest
                    if best_size == 0 or (width >= 24 and width <= 48):
                         best_size = width
                         best_chunk_pos = pos
                    
                    f.seek(cur_pos)

            if best_chunk_pos == 0:
                print("No image chunks found")
                return

            # Extract the Image
            f.seek(best_chunk_pos)
            # Read header again
            chunk_header = struct.unpack('<IIIIIIIII', f.read(36))
            width = chunk_header[4]
            height = chunk_header[5]
            
            print(f"Extracting {width}x{height} image...")
            
            pixels_data = f.read(width * height * 4)
            
            # XCursor pixels are ARGB, PIL Image.frombytes matches
            # Create Image
            img = Image.frombytes('RGBA', (width, height), pixels_data)
            
            # Save as BMP
            # Note: XCursor is pre-multiplied alpha? Usually just ARGB.
            # Convert to BGRA or whatever our BMP loader expects (PIL handles standard BMP)
            img.save(output_bmp, "BMP")
            print(f"Saved to {output_bmp}")

    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 extract_cursor.py <xcursor_file> <output_bmp>")
    else:
        extract_cursor(sys.argv[1], sys.argv[2])
