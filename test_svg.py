from PIL import Image
try:
    img = Image.open('icons/app/kora/apps/scalable/brave-desktop.svg')
    print("Success")
except Exception as e:
    print(f"Failed: {e}")
