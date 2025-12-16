
import os
import glob
from PIL import Image

def convert():
    # 1. Define Sources
    # Key: C Variable Name
    # Value: Tuple (Explicit Path or None, Artifact Prefix or None)
    
    # explicit paths relative to project root
    icns_dir = "icons/app"
    artifacts_dir = "/home/aakash/.gemini/antigravity/brain/39b29feb-3027-4065-b6e1-1de4582c7689/"
    
    config = {
        "icon_computer":    (os.path.join(icns_dir, "computerManager.icns"), None),
        "icon_folder":      (os.path.join(icns_dir, "folder.icns"), None),
        "icon_terminal":    (os.path.join(icns_dir, "terminal.icns"), None),
        "icon_text_editor": (None, "macos_text_editor_icon"),
        "icon_settings":    (None, "macos_settings_icon"),
        "icon_file":        (None, "macos_text_editor_icon"), # Reuse editor icon for generic files for now
    }
    
    header_path = "kernel/include/icons_data.h"
    
    print(f"Writing to {header_path}...")
    with open(header_path, "w") as f:
        f.write("#ifndef ICONS_DATA_H\n")
        f.write("#define ICONS_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write("#define ICON_WIDTH 48\n")
        f.write("#define ICON_HEIGHT 48\n\n")
        
        for var_name, (explicit_path, artifact_prefix) in config.items():
            src = None
            
            # 1. Try Explicit Path
            if explicit_path:
                if os.path.exists(explicit_path):
                    src = explicit_path
                else:
                    print(f"Warning: Explicit path {explicit_path} not found.")

            # 2. Try Artifact Search if no src yet
            if not src and artifact_prefix:
                candidates = [fn for fn in os.listdir(artifacts_dir) if fn.startswith(artifact_prefix) and fn.endswith(".png")]
                if candidates:
                    candidates.sort() # Latest
                    src = os.path.join(artifacts_dir, candidates[-1])
            
            if not src:
                print(f"Error: Could not find source for {var_name}")
                continue
                
            print(f"Converting {src} to {var_name}...")
            
            try:
                img = Image.open(src)
                
                # If ICNS, it might have multiple sizes. modify resize to be high quality
                # Image.open on ICNS usually loads the largest available or composited one.
                # We simply resize to 48x48 using LANCHOS for quality.
                img = img.resize((48, 48), Image.Resampling.LANCZOS)
                img = img.convert("RGBA")
                
                f.write(f"static const uint32_t {var_name}[48 * 48] = {{\n")
                
                data = list(img.getdata())
                chunk_size = 48
                for i in range(0, len(data), chunk_size):
                    chunk = data[i:i+chunk_size]
                    lines = []
                    for r, g, b, a in chunk:
                        val = (a << 24) | (r << 16) | (g << 8) | b
                        lines.append(f"0x{val:08X}")
                    f.write(",".join(lines) + ",")
                    f.write("\n")
                
                f.write("};\n\n")
            except Exception as e:
                print(f"Failed to process {src}: {e}")
            
        f.write("#endif\n")

if __name__ == "__main__":
    convert()
