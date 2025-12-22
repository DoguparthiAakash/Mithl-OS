<div align="center">

  <img src="OSLogo.png" alt="Mithl OS Logo" width="200" height="200">

  # Mithl OS 🚀
  **The Next-Generation Independent Operating System**

  [![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](https://github.com/your-username/Mithl-OS)
  [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
  [![Platform](https://img.shields.io/badge/Platform-x86__64%20%7C%20i686-blue)](https://github.com/DoguparthiAakash/Mithl-OS)
  [![PRs Welcome](https://img.shields.io/badge/PRs-welcome-orange.svg)](http://makeapullrequest.com)

  <p align="center">
    <b>Built from Scratch</b> • <b>No Linux Kernel</b> • <b>Glassmorphism UI</b>
  </p>

  [⬇️ Download ISO](#-getting-started) • [📖 Documentation](#-under-the-hood-technical-specs) • [🤝 Join the Devs](#-contributing)

</div>

---

## 🌟 Vision & Philosophy

**Mithl OS** is a passion project built entirely from the ground up to push the boundaries of custom operating systems.

Unlike many OSDev projects that stop at a command line, Mithl OS delivers a **Full Desktop Environment** immediately after boot. It is not based on Linux, Unix, or any existing kernel—every line of code, from the bootloader to the graphical interface, has been crafted to deliver a unique experience.

We prioritize:
* **Independence:** A completely custom kernel (C/Asm/Rust) and userspace.
* **Visual Polish:** A unique design language featuring glassmorphism, rounded corners, and smooth typography.
* **Modern Standards:** UEFI Boot (Multiboot2), ACPI Power Management, and VESA High-Res Graphics.

---

## 🎨 The Experience (Screenshots)

> *Place a GIF or Screenshot here showing the OS booting and opening a window.*

### 🖥️ The Desktop Environment
We built our own **Compositor** from scratch to avoid the bloat of X11 or Wayland.
* **Visuals:** Custom graphics engine supporting alpha blending and "Frosted Glass" effects.
* **Unified Workspace:** A centralized hub for launching apps.
* **Floating Dock:** Elegant application management.

### 📂 Productivity Tools
* **File Manager:** Grid view with ZFS-structure awareness (preparing for OpenZFS).
* **Mithl Text Editor:** A native coding tool with line numbers, status bars, and full keyboard support.

---

## 🔧 Under the Hood (Tech Stack)

For the engineers, here is what makes Mithl OS tick:

| Component | Technology | Description |
| :--- | :--- | :--- |
| **Kernel** | **C99 & Assembly** | The core foundation for speed and robustness. |
| **Safety** | **Rust** | Currently integrating Rust for memory-safe driver development. |
| **Boot** | **Multiboot2** | Supports both Legacy BIOS and UEFI systems. |
| **Memory** | **PMM & VMM** | Custom bitmap allocator and paging system. |
| **Drivers** | **ACPI & PS/2** | Custom parsers for power management and input handling. |

---

## 🚀 Getting Started

Want to run Mithl OS? It's easy!

### Prerequisites
You need a Linux environment (WSL2 works) with:
* `gcc` (or `i686-elf-gcc`), `nasm`, `make`
* `xorriso` (for ISO creation)
* `qemu-system-x86_64` (for emulation)

### Installation Steps
1.  **Clone the Repo**:
    ```bash
    git clone [https://github.com/DoguparthiAakash/Mithl-OS.git](https://github.com/DoguparthiAakash/Mithl-OS.git)
    cd Mithl-OS
    ```

2.  **Compile & Run (UEFI/BIOS Hybrid)**:
    ```bash
    make run-uefi
    ```

3.  **Generate ISO**:
    ```bash
    make iso
    ```

---

## 🗺️ Roadmap

We are building in public. Here is our current status:

- [x] **Core:** Bootloader & GDT/IDT Initialization
- [x] **Graphics:** VESA Linear Framebuffer & Custom Compositor
- [ ] **Filesystem:** True Read/Write support (FAT32/EXT2)
- [ ] **Multitasking:** Preemptive scheduling & Ring 3 User Mode
- [ ] **Connectivity:** Basic TCP/IP Network Stack
- [ ] **The Ultimate Goal:** Run Doom.

---

## 🤝 How to Contribute

**Mithl OS is open for everyone.** whether you are a kernel expert or a student learning C.

**We are specifically looking for help with:**
1.  **Rust Integration:** Helping us move more drivers to Rust.
2.  **UI Design:** Refining icons and window themes.
3.  **Testing:** Running the ISO on real hardware and reporting logs.

**Steps to Contribute:**
1.  Fork the repo.
2.  Create a branch (`git checkout -b feature/cool-thing`).
3.  Commit your changes.
4.  Open a Pull Request!

---

## ❤️ Community & License

**Mithl OS** is open-source software licensed under the **MIT License**.

> **A Note from the Creator:**
> *This OS was born out of pure curiosity. It is a non-profit, community-driven project. If you are willing to use the code, contribute to the kernel, or help us grow into a Linux-alternative community, you are heartily welcomed. Let's build something amazing together.*
>
> *Crafted with ❤️ and caffeine by Aakash and the Open Source Community.*
