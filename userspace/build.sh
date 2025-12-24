#!/bin/bash
set -e

# Config
CC=gcc
ASM=nasm
LD=ld

CFLAGS="-m32 -ffreestanding -fno-pie -nostdlib -fno-stack-protector -I./libc"
LDFLAGS="-m32 -nostdlib -T linker.ld"

echo "Building LibC..."
nasm -f elf32 libc/syscall.asm -o libc/syscall.o
nasm -f elf32 libc/crt0.asm -o libc/crt0.o
gcc $CFLAGS -c libc/stdlib.c -o libc/stdlib.o

echo "Building Hello App..."
gcc $CFLAGS -c apps/hello/hello.c -o apps/hello/hello.o

echo "Linking Hello App..."
ld -m elf_i386 -T linker.ld -o apps/hello/hello.elf libc/crt0.o libc/syscall.o libc/stdlib.o apps/hello/hello.o

echo "Done! Executable is at apps/hello/hello.elf"
