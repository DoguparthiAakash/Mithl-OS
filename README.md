<div align="center">

  <br />
  <img src="OSLogo.png" alt="Mithl OS Logo" width="300" height="300">
   
  <h1 align="center">Mithl OS 🚀</h1>

  <p align="center">
    <b>The Next-Generation Independent Operating System</b><br/>
    Built from scratch in C, Assembly & Rust. No Linux. No Unix. Just Code.
    <br />
    <br />
    <a href="https://github.com/DoguparthiAakash/Mithl-OS/releases"><strong>⬇️ Download ISO</strong></a>
    ·
    <a href="#-under-the-hood-technical-specs"><strong>📖 Read Docs</strong></a>
    ·
    <a href="#-contributing"><strong>🤝 Join Development</strong></a>
  </p>

  <p align="center">
    <a href="https://github.com/DoguparthiAakash/Mithl-OS/actions">
      <img src="https://img.shields.io/badge/build-passing-brightgreen?style=flat-square" alt="Build Status" />
    </a>
    <a href="https://opensource.org/licenses/MIT">
      <img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square" alt="License" />
    </a>
    <a href="https://github.com/DoguparthiAakash/Mithl-OS">
      <img src="https://img.shields.io/badge/Platform-x86__64%20%7C%20i686-blue?style=flat-square" alt="Platform" />
    </a>
    <a href="http://makeapullrequest.com">
      <img src="https://img.shields.io/badge/PRs-welcome-orange.svg?style=flat-square" alt="PRs Welcome" />
    </a>
  </p>
   
  <p align="center">
    <b>Built from Scratch</b> • <b>No Linux Kernel</b> • <b>Glassmorphism UI</b>
  </p>

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

## 🎨 The Experience

![Mithl OS Demo](demo.gif)

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
    This command builds the kernel, creates a bootable ISO, and launches QEMU.
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
- [ ] **Filesystem:** True Read/Write support (FAT32, exFAT, EXT4, Btrfs, ZFS)
- [ ] **Multitasking:** Preemptive scheduling & Ring 3 User Mode
- [ ] **Connectivity:** Basic TCP/IP Network Stack
- [ ] **The Least Goal:** Run Doom.
- [ ] **The Actual Goal:** To make it usable by all, like Linux and other major OSs, without dissatisfaction.
- [ ] **The Ultimate Goal:** Handle and use extreme complex tasks without panic or lag.

---

## 🤝 How to Contribute

**Mithl OS is open for everyone**, whether you are a kernel expert or a student learning C.

**We are specifically looking for help with:**
1.  **Rust Integration:** Helping us move more drivers to Rust.
2.  **UI Design:** Refining icons and window themes.
3.  **Testing:** Running the ISO on real hardware and reporting logs.
4.  **Core Development:** Adding functions and features to make it happen.
5.  **App Ecosystem:** Building, developing, and converting apps to run natively on the OS.

**Steps to Contribute:**
1.  Fork the repo.
2.  Create a branch (`git checkout -b feature/cool-thing`).
3.  Commit your changes.
4.  Open a Pull Request!

---

## ❤️ Community & License

**Mithl OS** is open-source software licensed under the **MIT License**.

> **A Note from the Creator:**
> *This OS was born out of pure curiosity. It is a non-profit, community-driven project. If you are willing to use the code, contribute to the kernel, or help us grow into something like Linux or other platforms, you are heartily welcomed. Let's build something amazing together.*
>
> *Crafted with ❤️ and caffeine by Aakash and the Open Source Community.*
