<p align="center">
  <img src="OSLogo.png" alt="Mithl OS Logo" width="520"/>
</p>

<h1 align="center">💻 Mithl OS</h1>

<p align="center"><em>A fully from-scratch operating system featuring a custom-built kernel and a modern graphical interface.</em></p>

---

### 🧠 About the Project
**Mithl OS** is an entirely custom-built operating system — designed from the ground up, without depending on any existing OS architecture.  
From the **kernel** to the **GUI system**, every component has been written manually to ensure total control, performance, and design freedom.

> “An entirely built-from-scratch OS — to make things right.”

---

### ✨ Key Features

#### 🪟 Modern GUI System
- 🧩 **Custom Interface:** Designed from scratch with a clean, modern layout  
- 🪟 **Window Management:** Draggable and resizable windows with control buttons  
- 🎨 **Interactive Elements:** Buttons, labels, panels, hover, and click effects  
- 🖌️ **Professional Color Scheme:** Smooth, elegant visual experience  

#### 🖼️ Graphics Engine
- 📺 **High-Resolution Mode:** 1024×768×32 graphics  
- 🧮 **Drawing Primitives:** Lines, rectangles, circles, text rendering  
- 🌈 **ARGB Color Support:** Transparency, blending, and pixel-level control  
- ⚙️ **Framebuffer Graphics:** Direct pixel rendering for fast performance  

#### ⚙️ Kernel Features
- 🔩 **Multiboot Compatible:** Boots seamlessly via GRUB  
- 💾 **Graphics Mode:** Switches from VGA text to graphical mode  
- 🖱️ **Event Handling:** Mouse and keyboard input system  
- 🧠 **Memory Management:** Lightweight allocator for GUI components  

---

### 🏗️ Building Mithl OS

#### 🧰 Prerequisites
Ensure the following tools are installed:
- **NASM** – Netwide Assembler  
- **GCC (32-bit)**  
- **LD** – GNU Linker  
- **GRUB Tools** – for ISO image creation
📂 Development Access
📁 Google Drive (All Files): Open Folder

📦 Direct ZIP File: Download Here

🧩 Tech Stack
Language: Assembly, C

Bootloader: GRUB (Multiboot Specification)

Build Tools: Make, NASM, GCC, LD

Architecture: 32-bit (x86)

📜 License
© 2025 Aakash Doguparthi — All rights reserved.


#### ⚡ Build Commands
```bash
# Build the kernel
make

# Create bootable ISO
make iso

# Clean build files
make clean

```
<p align="center">Made with ❤️ and low-level magic by <b>Aakash Doguparthi</b></p> 
