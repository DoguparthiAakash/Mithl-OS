# Mithl-OS Hybrid Root Makefile

.PHONY: all core gui clean

all: core gui

core:
	$(MAKE) -C modules/mithl_core

gui:
	$(MAKE) -C userspace/gui

clean:
	$(MAKE) -C modules/mithl_core clean
	$(MAKE) -C userspace/gui clean
	rm -rf build boot

iso: core gui
	./scripts/build_iso.sh

run: iso
	qemu-system-x86_64 -kernel ./kernel.img \
		-initrd boot/initramfs.cpio.gz \
		-append "console=ttyS0 vga=792" \
		-m 512 \
		-serial stdio \
		-vga std
