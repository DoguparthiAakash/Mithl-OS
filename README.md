<p align="center">
  <img src="OSLogo.png" ... >
</p>


# Mithl OS 🚀
> *A next-generation, independent operating system written in C and Assembly (with a sprinkle of Rust & Python).*

Welcome to **Mithl OS**! This is a passion project built entirely from the ground up, designed to push the boundaries of what a custom operating system can be. It is not based on Linux, Unix, or any existing kernel—every line of code, from the bootloader to the graphical interface, has been crafted from scratch to deliver a unique and powerful computing experience.



---

## 🌟 Vision & Philosophy
Mithl OS was born out of a desire to understand *exactly* how a computer works, from the first instruction of the bootloader to drawing pixel-perfect windows on the screen. 

Our goal is simple: **Total Control & Modern Design.**

Unlike many OSDev projects that stop at a command line, Mithl OS pushes for a full **Desktop Environment** experience immediately after boot. We prioritize:
*   **Independence**: A completely custom kernel and userspace.
*   **Visual Polish**: A unique design language featuring glassmorphism, rounded corners, and smooth typography.
*   **Versatility**: Designed to support both **32-bit (i686)** and **64-bit (x86_64)** architectures.
*   **Modern Standards**: UEFI Boot (via Multiboot2), ACPI Power Management, and VESA High-Res Graphics.

---

## 🎨 Key Features

### 🖥️ The Desktop Experience
The desktop environment (Compositor) is the jewel of Mithl OS.
*   **Custom Graphics Engine**: Written from scratch to support alpha blending, transparency, and "Frosted Glass" effects without relying on external libraries.
*   **Unified Workspace**: A centralized hub for launching apps and managing tasks.
*   **Floating Dock**: An elegant, floating application dock for quick access.
*   **Intuitive Controls**: familiar but distinct window management controls designed for efficiency.

### 📂 File Manager
A fully graphical file manager built for productivity.
*   **Grid View**: Icons are laid out in a responsive, auto-arranging grid.
*   **Navigation**: Seamless directory traversal with intuitive Up/Back navigation.
*   **ZFS Integration**: While the kernel runs on a RAM disk for now, the UI is built to respect ZFS structures (zroot), preparing for a future port of OpenZFS.

### 📝 Mithl Text Editor
A native, lightweight text editor for coding and notes.
*   **Real Typing**: Full keyboard support for standard input, Backspace, and Enter.
*   **Cursor Control**: Fluid cursor navigation using arrow keys.
*   **Development Ready**: Includes line numbers, a status bar, and a productivity-focused toolbar.

### ⚙️ System Settings
*   **Adaptive Layout**: A settings application that features a modern, card-based layout.
*   **Power Management**: Fully functional **Shutdown** and **Restart** capabilities powered by a custom ACPI driver.

---

## 🔧 Under the Hood (Technical Specs)

For the engineers and enthusiasts, here is what makes Mithl OS tick:

### Kernel Core
*   **Architecture**: Dual-Architecture Support (x86 / x64).
*   **Boot Protocol**: Multiboot2 implementation.
*   **Memory Management**: Advanced paging system, PMM (Physical Memory Manager) with a custom bitmap allocator, and VMM (Virtual Memory Manager).
*   **Drivers**:
    *   **ACPI**: Custom parser for RSDP/FADT tables to handle power events reliably.
    *   **PS/2**: Full keyboard and mouse driver with scancode mapping.
    *   **Serial**: COM1 logging for debugging.
    *   **VGA/VESA**: Linear Framebuffer access.

### Hybrid Language Approach
*   **C (C99)**: The core kernel provides a robust and fast foundation.
*   **Assembly (NASM)**: Used for critical low-level operations like GDT flushing and Interrupt Service Routines (ISRs).
*   **Rust**: We are integrating Rust to bring modern memory safety to driver development, linked directly into the kernel.

### Graphics Engine
We don't use X11, Wayland, or any existing display server. We built our own **Compositor**:
*   **Double Buffering**: Eliminates screen tearing by drawing to a backbuffer before flipping to VRAM.
*   **Dirty Rectangles**: Optimized rendering engine that only redraws parts of the screen that changed.
*   **Fonts**: Custom rendering engine for high-quality typography.

---

## 🚀 Getting Started

Want to run Mithl OS? It's easy!

### Prerequisites
You need a Linux environment (WSL2 works too) with the following installed:
*   `gcc` (Cross-compiler `i686-elf-gcc` recommended, but standard `gcc` works)
*   `nasm`
*   `make`
*   `xorriso` (for ISO creation)
*   `qemu-system-i386` or `qemu-system-x86_64` (for testing)
*   `grub-pc-bin` / `grub-common`

### Building & Running
1.  **Clone the Repo**:
    ```bash
    git clone https://github.com/your-username/Mithl-OS.git
    cd Mithl-OS
    ```

2.  **Compile & Run (UEFI/BIOS Hybrid)**:
    This command builds the kernel, creates a bootable ISO, and launches QEMU.
    ```bash
    make run-uefi
    ```

3.  **Just ISO**:
    If you want to run it in VirtualBox or VMware:
    ```bash
    make iso
    ```
    This generates `Mithl.iso`.

### Debugging
If you run into issues, check the `serial.log` or view the terminal output. ACPI and Memory logs are dumped to COM1.

---

## 🗺️ Roadmap

We are just getting started. Here is what's coming next:
- [ ] **Filesystem**: True Read/Write support (FAT32 or EXT2 implementation).
- [ ] **User Mode**: Ring 3 switching for true process isolation.
- [ ] **Multitasking**: Preemptive scheduling (currently cooperative-ish via event loops).
- [ ] **64-bit Port**: Finalizing the switch to long mode.
- [ ] **Network Stack**: Basic TCP/IP support.
- [ ] **Doom**: Yes, we will eventually run Doom.

---

## 🤝 Contributing

We love open source! If you want to fix a bug, add a driver, or redesign an icon:
1.  Fork the repo.
2.  Create a branch (`git checkout -b feature/cool-thing`).
3.  Commit your changes.
4.  Open a Pull Request!

---

## 📜 License
Mithl OS is open-source software licensed under the **MIT License**. Feel free to use it, learn from it, and break it!

## Note
*This OS is created because of my curiosity. its "Neither Funded Nor Earning from it".*

*If you willing to use the code or contrubuting to develop the OS to the nextlevel to make it everyone to use, you are hartly Welcomed and make contributions, suggestions and make the os to grow like the linux community.*

*Crafted with ❤️ and a lot of caffeine by Aakash and the Open Source Community.*
