# Mithl OS - From-Scratch OS with a Modern GUI

**Mithl OS** is a completely custom-built operating system kernel that features a modern graphical user interface, designed from scratch without relying on existing OS architectures. While the interface design is clean and modern, the entire system — from the kernel to the GUI — has been developed independently from the ground up.

## Features

### Modern GUI System
- **Custom-Built Interface**: Entire GUI system built from scratch with a clean, modern look
- **Window Management**: Draggable, resizable windows with title bars and control buttons
- **Interactive Elements**: Buttons, labels, and panels with hover and click interactions
- **Color Scheme**: Professional color palette for a polished user experience

### Graphics System
- **High-Resolution Support**: 1024x768x32 graphics mode
- **Drawing Primitives**: Lines, rectangles, circles, and text rendering
- **Color Management**: ARGB color support with blending and effects
- **Framebuffer Graphics**: Direct pixel manipulation for smooth rendering

### Kernel Features
- **Multiboot Compatible**: Boots via GRUB bootloader
- **Graphics Mode**: Switches from VGA text mode to graphical mode
- **Event Handling**: Mouse and keyboard input processing
- **Memory Management**: Simple memory allocation for GUI elements

## Building

### Prerequisites
- NASM (Netwide Assembler)
- GCC with 32-bit support
- LD (GNU Linker)
- GRUB tools (for ISO creation)

### For Development And Usage
Files Access: url:https://drive.google.com/drive/folders/1o5yuezvedfr3F1nlErPffO1PP5ai2NOq?usp=drive_link

Direct Zip  : url:https://drive.google.com/file/d/1ajlRBfFtDubOjY79e0DU1LOFPrhC0XJk/view?usp=drive_link

### Build Commands
```bash
# Build the kernel
make

# Build and create bootable ISO
make iso

# Clean build files
make clean

