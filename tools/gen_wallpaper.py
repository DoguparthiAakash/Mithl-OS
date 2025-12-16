from PIL import Image, ImageDraw
import math

def generate_wallpaper(path):
    width = 1024
    height = 768
    img = Image.new("RGB", (width, height), "#006060") # Dark Teal Base
    draw = ImageDraw.Draw(img)
    
    # Draw some "Paper Cut" waves
    # Wave 1
    points1 = [(0, 300), (300, 200), (600, 400), (1024, 200), (1024, 768), (0, 768)]
    draw.polygon(points1, fill="#008080") # Medium Teal
    
    # Wave 2
    points2 = [(0, 500), (400, 400), (700, 600), (1024, 500), (1024, 768), (0, 768)]
    draw.polygon(points2, fill="#00A0A0") # Light Teal
    
    # Wave 3 (Accent)
    points3 = [(0, 650), (500, 600), (1024, 700), (1024, 768), (0, 768)]
    draw.polygon(points3, fill="#00C0C0") # Bright Teal
    
    # Add some circles/bubbles
    draw.ellipse((800, 100, 900, 200), fill="#FFFFFF30", outline=None) # Translucent White? No alpha in RGB mode
    # Simulate alpha by blending? 
    # Just draw solid nice shapes
    
    img.save(path)
    print(f"Generated wallpaper at {path}")

if __name__ == "__main__":
    generate_wallpaper("wallpapers/generated.png")
