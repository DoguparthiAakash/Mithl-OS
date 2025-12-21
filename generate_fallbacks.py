from PIL import Image, ImageDraw

def create_color_icon(name, color):
    img = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Draw rounded rect
    draw.rounded_rectangle([2, 2, 30, 30], radius=5, fill=color)
    img.save(f"icon_{name}.png")
    print(f"Created icon_{name}.png")

if __name__ == "__main__":
    create_color_icon("home", "#4A90E2")      # Blue
    create_color_icon("desktop", "#50E3C2")   # Teal
    create_color_icon("documents", "#F5A623") # Orange
    create_color_icon("downloads", "#4A90E2") # Blue
    create_color_icon("music", "#BD10E0")     # Purple
    create_color_icon("pictures", "#7ED321")  # Green
    create_color_icon("videos", "#D0021B")    # Red
    create_color_icon("file", "#FFFFFF")      # White
