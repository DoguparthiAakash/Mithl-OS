# Tools
ASM = nasm
CC  = gcc
CXX = g++
LD  = gcc

# Flags
ASMFLAGS = -f elf32
CFLAGS   = -m32 -ffreestanding -fno-pie -nostdlib -nostdinc -Wall -Wextra -mno-sse -mno-sse2 -mno-mmx -mno-80387 -I./kernel/include -I./kernel
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -fno-use-cxa-atexit
LDFLAGS  = -m32 -nostdlib -no-pie -Wl,-T,linker/linker.ld -o kernel.elf
# Sources
ASM_SOURCES = boot/boot.asm kernel/interrupts.asm kernel/gdt_flush.asm kernel/power.asm
C_SOURCES   = kernel/kernel.c \
              kernel/gdt.c \
              kernel/idt.c \
              kernel/graphics.c \
              kernel/gui.c \
              kernel/filesystem.c \
              kernel/keyboard.c \
              kernel/mouse.c \
              kernel/ata.c \
              kernel/memory.c \
              kernel/string.c \
              kernel/event.c \
              kernel/input.c \
              kernel/ports.c \
              kernel/console.c \
              kernel/rtc.c \
              kernel/vga.c \
              kernel/list.c \
              desktop_env/compositor.c \
              kernel/apps/terminal/terminal.c \
              kernel/apps/terminal/commands.c \
              kernel/apps.c \
              kernel/apps/text_editor/text_editor.c \
              kernel/apps/file_manager/file_manager.c \
              kernel/vfs.c \
              kernel/ramfs.c \
              kernel/process.c \
              windowmanager/mithl_wm.c \
              kernel/mm/pmm.c \
              kernel/mm/vmm.c \
              kernel/mm/zram.c \
              kernel/elf_loader.c \
              kernel/apps/settings/settings.c \
              kernel/graphics/triangle.c \
              kernel/gpu/gpu.c \
              kernel/boot_adapter.c \
              kernel/acpi.c \
              kernel/theme.c
CXX_SOURCES = kernel/lib/cxx_runtime.cpp

OBJECTS     = $(ASM_SOURCES:.asm=.o) $(C_SOURCES:.c=.o) $(CXX_SOURCES:.cpp=.o)
RUST_LIB    = kernel/rust/target/i686-unknown-linux-gnu/release/libmithl_rust.a

# Default target
all: kernel.elf

# Build the kernel ELF (Multiboot)
kernel.elf: $(OBJECTS) $(RUST_LIB)
	$(LD) $(LDFLAGS) $(OBJECTS) $(RUST_LIB)

# Assembly source
%.o: %.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

# C source
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# C++ source
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean
clean:
	rm -f $(OBJECTS) kernel.elf Mithl.iso
	rm -rf bootiso
	cd kernel/rust && . "$$HOME/.cargo/env" && cargo clean

# Build Rust static library
$(RUST_LIB):
	cd kernel/rust && . "$$HOME/.cargo/env" && RUSTFLAGS="-C target-feature=-sse,-sse2,-mmx -C soft-float -C relocation-model=static" cargo build --target i686-unknown-linux-gnu --release

# Run in QEMU (BIOS mode)
run: iso
	qemu-system-i386 -cdrom Mithl.iso -serial stdio

# Create bootable ISO (BIOS only for now)
iso: kernel.elf
	rm -rf bootiso
	mkdir -p bootiso/boot/grub
	cp kernel.elf bootiso/boot/
	
	# GRUB config for BIOS
	echo 'set timeout=5' > bootiso/boot/grub/grub.cfg
	echo 'set default=0' >> bootiso/boot/grub/grub.cfg
	echo '' >> bootiso/boot/grub/grub.cfg
	echo 'insmod all_video' >> bootiso/boot/grub/grub.cfg
	echo 'set gfxpayload=keep' >> bootiso/boot/grub/grub.cfg
	echo '' >> bootiso/boot/grub/grub.cfg
	echo 'menuentry "Mithl OS" {' >> bootiso/boot/grub/grub.cfg
	echo '  multiboot2 /boot/kernel.elf' >> bootiso/boot/grub/grub.cfg
	echo '  boot' >> bootiso/boot/grub/grub.cfg
	echo '}' >> bootiso/boot/grub/grub.cfg
	
	# Create ISO
	grub-mkrescue -o Mithl.iso bootiso
	@echo "ISO created: Mithl.iso (BIOS boot only)"
	@echo "Note: UEFI support requires kernel changes to support Multiboot2 protocol"

# Run in QEMU (UEFI mode) - requires OVMF firmware
run-uefi: iso
	@if [ -f /usr/share/ovmf/OVMF.fd ]; then \
		qemu-system-x86_64 -cdrom Mithl.iso -bios /usr/share/ovmf/OVMF.fd -serial stdio; \
	elif [ -f /usr/share/edk2-ovmf/x64/OVMF.fd ]; then \
		qemu-system-x86_64 -cdrom Mithl.iso -bios /usr/share/edk2-ovmf/x64/OVMF.fd -serial stdio; \
	else \
		echo "OVMF firmware not found. Install with: sudo apt install ovmf"; \
		echo "Falling back to BIOS mode..."; \
		qemu-system-x86_64 -cdrom Mithl.iso -serial stdio; \
	fi
