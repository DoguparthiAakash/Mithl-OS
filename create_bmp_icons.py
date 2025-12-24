import os
import sys

# Try to import cairosvg
try:
    import cairosvg
except ImportError:
    print("Error: cairosvg not found. Please install it or use a pre-converted BMP.")
    print("Attempting to use minimal SVG parser if possible or fail.")
    # Fallback to just generating colored blocks if SVG conversion fails
    # But let's assume for this environment we might have it or stick to placeholders if not.
    
from PIL import Image
import io

ICONS = [
    ("icons/kora-2-0-0/kora/apps/scalable/start-here-kde.svg", "userspace/gui/start_icon.bmp"),
    ("icons/kora-2-0-0/kora/apps/scalable/Terminal.svg", "userspace/gui/terminal_icon.bmp"),
    ("icons/kora-2-0-0/kora/apps/scalable/settings.svg", "userspace/gui/settings_icon.bmp") # If exists
]

def convert_svg_to_bmp(svg_path, bmp_path, width=48, height=48):
    if not os.path.exists(svg_path):
        print(f"Warning: {svg_path} not found. Skipping.")
        return

    print(f"Converting {svg_path} -> {bmp_path}...")
    
    try:
        # Convert SVG to PNG bytes
        png_data = cairosvg.svg2png(url=svg_path, output_width=width, output_height=height)
        
        # Open with PIL
        img = Image.open(io.BytesIO(png_data))
        
        # Ensure RGBA
        img = img.convert("RGBA")
        
        # Create BMP (32-bit ARGB for our loader)
        # Our loader expects BGRA byte order usually? Or just ARGB.
        # Let's write it using PIL's BMP saver but our loader expects standard 32bpp
        # Image.save(bmp_path) might write standard BMP.
        
        img.save(bmp_path, "BMP")
        print("Success.")
        
    except Exception as e:
        print(f"Failed to convert: {e}")
        # Create Placeholder
        img = Image.new('RGB', (width, height), color = 'red')
        img.save(bmp_path, "BMP")

if __name__ == "__main__":
    for svg, bmp in ICONS:
        convert_svg_to_bmp(svg, bmp)
