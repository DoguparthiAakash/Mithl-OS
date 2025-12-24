from PIL import Image, ImageDraw, ImageFont
import os

ICONS = {
    "userspace/gui/start_icon.bmp": "start",
    "userspace/gui/terminal_icon.bmp": "term",
    "userspace/gui/settings_icon.bmp": "settings"
}

def create_icon(path, type):
    size = 48
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    if type == "start":
        # Blue Circle with K (Kora-like)
        draw.ellipse((2, 2, size-2, size-2), fill="#5E81AC", outline="#81A1C1")
        # Draw 'M' for Mithl (since we can't do complex vector K)
        # draw.text((15, 10), "M", fill="white") # Need font, fallback to shapes
        draw.rectangle((18, 12, 22, 36), fill="white")
        draw.rectangle((26, 12, 30, 36), fill="white")
        draw.polygon([(18,12), (24,24), (30,12)], fill="white")
        
    elif type == "term":
        # Dark Box with >_
        draw.rectangle((4, 4, size-4, size-4), fill="#2E3440", outline="#4C566A")
        # >
        draw.line((12, 16, 20, 24), fill="#A3BE8C", width=3)
        draw.line((20, 24, 12, 32), fill="#A3BE8C", width=3)
        # _
        draw.line((24, 32, 32, 32), fill="#A3BE8C", width=3)
        
    elif type == "settings":
        # Grey Gear-ish circle
        draw.ellipse((4, 4, size-4, size-4), fill="#4C566A", outline="#D8DEE9")
        draw.ellipse((16, 16, size-16, size-16), fill="#2E3440", outline="#D8DEE9")
        
    img.save(path, "BMP")
    print(f"Generated {path}")

if __name__ == "__main__":
    for path, type in ICONS.items():
        create_icon(path, type)
