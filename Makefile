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
