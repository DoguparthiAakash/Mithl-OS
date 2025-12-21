# Tools
ASM = nasm
CC  = gcc
CXX = g++
LD  = gcc

# Flags
ASMFLAGS = -f elf32
CFLAGS   = -m32 -ffreestanding -fno-pie -nostdlib -nostdinc -Wall -Wextra -mno-sse -mno-sse2 -mno-mmx -mno-80387 -I./kernel/include -I./kernel -I./kernel/include/libc_shim -I./games/DOOM-master/linuxdoom-1.10 -DDOOM_MITHL -DNORMALUNIX=1 -DLINUX=1
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -fno-use-cxa-atexit
LDFLAGS  = -m32 -nostdlib -no-pie -Wl,-T,linker/linker.ld -o kernel.elf
# Sources
ASM_SOURCES = boot/boot.asm kernel/interrupts.asm kernel/gdt_flush.asm kernel/power.asm kernel/arch/i386/process.asm
C_SOURCES   = kernel/kernel.c \
              kernel/gdt.c \
              kernel/idt.c \
              kernel/graphics.c \
              kernel/gui.c \
              kernel/gui_dialog.c \
              kernel/filesystem.c \
              kernel/keyboard.c \
              kernel/mouse.c \
              kernel/ata.c \
              kernel/memory.c \
              kernel/string.c \
              kernel/event.c \
              kernel/input.c \
              kernel/ports.c \
              kernel/pit.c \
              kernel/console.c \
              kernel/rtc.c \
              kernel/acpi.c \
              kernel/vga.c \
              kernel/list.c \
              kernel/syscall.c \
              desktop_env/compositor.c \
              kernel/apps/terminal/terminal.c \
              kernel/apps/terminal/commands.c \
              kernel/apps.c \
              kernel/apps/text_editor/text_editor.c \
              kernel/apps/file_manager/file_manager.c \
              kernel/fs/fat32/fat32.c \
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
              kernel/theme.c \
              kernel/apps/doom/i_mithl.c \
              kernel/apps/doom/libc_doom.c \
              games/DOOM-master/linuxdoom-1.10/am_map.c \
              games/DOOM-master/linuxdoom-1.10/d_items.c \
              games/DOOM-master/linuxdoom-1.10/d_main.c \
              games/DOOM-master/linuxdoom-1.10/d_net.c \
              games/DOOM-master/linuxdoom-1.10/doomdef.c \
              games/DOOM-master/linuxdoom-1.10/doomstat.c \
              games/DOOM-master/linuxdoom-1.10/dstrings.c \
              games/DOOM-master/linuxdoom-1.10/f_finale.c \
              games/DOOM-master/linuxdoom-1.10/f_wipe.c \
              games/DOOM-master/linuxdoom-1.10/g_game.c \
              games/DOOM-master/linuxdoom-1.10/hu_lib.c \
              games/DOOM-master/linuxdoom-1.10/hu_stuff.c \
              games/DOOM-master/linuxdoom-1.10/info.c \
              games/DOOM-master/linuxdoom-1.10/m_argv.c \
              games/DOOM-master/linuxdoom-1.10/m_bbox.c \
              games/DOOM-master/linuxdoom-1.10/m_cheat.c \
              games/DOOM-master/linuxdoom-1.10/m_fixed.c \
              games/DOOM-master/linuxdoom-1.10/m_menu.c \
              games/DOOM-master/linuxdoom-1.10/m_misc.c \
              games/DOOM-master/linuxdoom-1.10/m_random.c \
              games/DOOM-master/linuxdoom-1.10/m_swap.c \
              games/DOOM-master/linuxdoom-1.10/p_ceilng.c \
              games/DOOM-master/linuxdoom-1.10/p_doors.c \
              games/DOOM-master/linuxdoom-1.10/p_enemy.c \
              games/DOOM-master/linuxdoom-1.10/p_floor.c \
              games/DOOM-master/linuxdoom-1.10/p_inter.c \
              games/DOOM-master/linuxdoom-1.10/p_lights.c \
              games/DOOM-master/linuxdoom-1.10/p_map.c \
              games/DOOM-master/linuxdoom-1.10/p_maputl.c \
              games/DOOM-master/linuxdoom-1.10/p_mobj.c \
              games/DOOM-master/linuxdoom-1.10/p_plats.c \
              games/DOOM-master/linuxdoom-1.10/p_pspr.c \
              games/DOOM-master/linuxdoom-1.10/p_saveg.c \
              games/DOOM-master/linuxdoom-1.10/p_setup.c \
              games/DOOM-master/linuxdoom-1.10/p_sight.c \
              games/DOOM-master/linuxdoom-1.10/p_spec.c \
              games/DOOM-master/linuxdoom-1.10/p_switch.c \
              games/DOOM-master/linuxdoom-1.10/p_telept.c \
              games/DOOM-master/linuxdoom-1.10/p_tick.c \
              games/DOOM-master/linuxdoom-1.10/p_user.c \
              games/DOOM-master/linuxdoom-1.10/r_bsp.c \
              games/DOOM-master/linuxdoom-1.10/r_data.c \
              games/DOOM-master/linuxdoom-1.10/r_draw.c \
              games/DOOM-master/linuxdoom-1.10/r_main.c \
              games/DOOM-master/linuxdoom-1.10/r_plane.c \
              games/DOOM-master/linuxdoom-1.10/r_segs.c \
              games/DOOM-master/linuxdoom-1.10/r_sky.c \
              games/DOOM-master/linuxdoom-1.10/r_things.c \
              games/DOOM-master/linuxdoom-1.10/s_sound.c \
              games/DOOM-master/linuxdoom-1.10/sounds.c \
              games/DOOM-master/linuxdoom-1.10/st_lib.c \
              games/DOOM-master/linuxdoom-1.10/st_stuff.c \
              games/DOOM-master/linuxdoom-1.10/tables.c \
              games/DOOM-master/linuxdoom-1.10/v_video.c \
              games/DOOM-master/linuxdoom-1.10/w_wad.c \
              games/DOOM-master/linuxdoom-1.10/wi_stuff.c \
              games/DOOM-master/linuxdoom-1.10/z_zone.c
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
	$(CC) $(CFLAGS) -c $< -o $@

# Doom-specific rule (Needs C89/gnu89 for false/true enum)
games/DOOM-master/linuxdoom-1.10/%.o: games/DOOM-master/linuxdoom-1.10/%.c
	$(CC) $(CFLAGS) -std=gnu89 -c $< -o $@

kernel/apps/doom/%.o: kernel/apps/doom/%.c
	$(CC) $(CFLAGS) -std=gnu89 -c $< -o $@

# C++ source
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean
disk.img:
	dd if=/dev/zero of=disk.img bs=1M count=32
	mkfs.vfat -F 32 disk.img

clean:
	rm -f $(OBJECTS) kernel.elf Mithl.iso
	rm -rf bootiso
	cd kernel/rust && . "$$HOME/.cargo/env" && cargo clean

# Build Rust static library
$(RUST_LIB):
	cd kernel/rust && . "$$HOME/.cargo/env" && RUSTFLAGS="-C target-feature=-sse,-sse2,-mmx -C soft-float -C relocation-model=static" cargo build --target i686-unknown-linux-gnu --release

# Run in QEMU (BIOS mode)
run: iso
	qemu-system-x86_64 -cdrom Mithl.iso -serial stdio

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
	# Add Modules
	if [ -f DOOM1.WAD ]; then \
	  cp DOOM1.WAD bootiso/boot/; \
	  echo '  module2 /boot/DOOM1.WAD DOOM1.WAD' >> bootiso/boot/grub/grub.cfg; \
	fi
	
	# Add Hello App
	if [ -f userspace/apps/hello/hello.elf ]; then \
	  cp userspace/apps/hello/hello.elf bootiso/boot/; \
	  echo '  module2 /boot/hello.elf hello.elf' >> bootiso/boot/grub/grub.cfg; \
	fi
	
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
		qemu-system-x86_64 -cdrom Mithl.iso -serial stdio -hda disk.img; \
	fi
