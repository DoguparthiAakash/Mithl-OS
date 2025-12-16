
import os
from PIL import Image

def convert_icons():
    # Map 'Source Filename' -> 'C Variable Name'
    mapping = {
        "computerManager.icns": "icon_app_computer",
        "filemanager.icns": "icon_app_filemanager",
        "notepad.icns": "icon_app_notepad",
        "settings.icns": "icon_app_settings",
        "terminal.icns": "icon_app_terminal"
    }
    
    src_dir = "icons/app"
    dst_file = "kernel/include/app_icons.h"
    
    print(f"Generating {dst_file}...")
    
    with open(dst_file, "w") as f:
        f.write("#ifndef APP_ICONS_H\n")
        f.write("#define APP_ICONS_H\n\n")
        f.write("#include <stdint.h>\n\n")
        
        for filename, var_name in mapping.items():
            path = os.path.join(src_dir, filename)
            if not os.path.exists(path):
                print(f"Warning: {path} not found, skipping.")
                continue
                
            print(f"Processing {filename} -> {var_name}")
            try:
                img = Image.open(path)
                
                # Check for sizes if ICNS
                # Pillow might open the largest available size by default or specific.
                # Ideally we want 48x48 or larger and resize down.
                # For now let's just resize whatever we get to 48x48.
                img = img.resize((48, 48), Image.Resampling.LANCZOS)
                img = img.convert("RGBA")
                
                data = list(img.getdata())
                
                f.write(f"// {filename}\n")
                f.write(f"static const uint32_t {var_name}[48 * 48] = {{\n")
                
                lines = []
                for r, g, b, a in data:
                    # ARGB format (0xAARRGGBB) for Mithl OS
                    val = (a << 24) | (r << 16) | (g << 8) | b
                    lines.append(f"0x{val:08X}")
                
                # Write in chunks to render nicer files? Or just big blob.
                # Doing it slightly dense to save IO ops
                chunk_size = 16 # 16 pixels per line
                for i in range(0, len(lines), chunk_size):
                    chunk = lines[i:i+chunk_size]
                    f.write(",".join(chunk) + ",\n")
                    
                f.write("};\n\n")
                
            except Exception as e:
                print(f"Error processing {filename}: {e}")
                
        f.write("#endif // APP_ICONS_H\n")
        
    print("Done.")

if __name__ == "__main__":
    convert_icons()
