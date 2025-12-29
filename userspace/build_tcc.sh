#!/bin/bash
set -e

# Config
TCC_SRC=../tcc_build
GCC_INC=$(gcc -m32 -print-file-name=include)
CFLAGS="-m32 -ffreestanding -fno-pie -nostdlib -fno-stack-protector -nostdinc -I./libc -I$GCC_INC -I$TCC_SRC -I$TCC_SRC/include -DTCC_TARGET_I386 -DCONFIG_TCC_STATIC -DCONFIG_TCC_SEMLOCK=0 -DCONFIG_TCC_BACKTRACE=0 -DTCC_VERSION=\"0.9.27\" -Wall -Wno-int-conversion"

# Note: We are NOT using ONE_SOURCE=1 because we want to inspect and control the build. 
# But tcc.c default ONE_SOURCE=1. We must define ONE_SOURCE=0 to compile separately, 
# OR let it be 1 and compile just tcc.c.
# Compiling just tcc.c is faster/easier IF it works.
# Let's try ONE_SOURCE=1 first (default in tcc.c) but pass -DONE_SOURCE=1 explicitly to be sure.

# Actually, if I compile tcc.c, and it includes libtcc.c...
# I need to see libtcc.c.

# If separate:
# gcc ... -c tcc.c -DONE_SOURCE=0 ...
# gcc ... -c libtcc.c ...

# Let's wait for view_file result to decide.
# But I can write the script now assuming ONE_SOURCE=1 for simplicity if verified.
# I will use separate compilation logic in the script but comment it out, defaulting to single file if verified.

echo "Rebuilding LibC..."
./build.sh

echo "Building TCC..."

# Attempt 1: Single Source Build
gcc $CFLAGS -DONE_SOURCE=1 -c $TCC_SRC/tcc.c -o tcc_single.o

# GCC_LIB=$(gcc -m32 -print-libgcc-file-name)
LIBGCC_DIR=$(gcc -m32 -print-libgcc-file-name | xargs dirname)
echo "LibGCC Dir: $LIBGCC_DIR"

echo "Linking TCC..."
ld -m elf_i386 -T linker.ld -o apps/tcc.elf libc/crt0.o libc/syscall.o libc/stdlib.o libc/string.o libc/malloc.o libc/stdio.o tcc_single.o
# Note: libc objects are already built by build.sh

echo "Done! apps/tcc.elf created."
