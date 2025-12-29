#!/bin/bash
set -e

# Create Sysroot Structure
echo "Creating sysroot..."
rm -rf sysroot
mkdir -p sysroot/bin
mkdir -p sysroot/usr/include/sys
mkdir -p sysroot/usr/lib

# Apps
echo "Copying Apps..."
cp userspace/apps/tcc.elf sysroot/bin/tcc
# Copy hello for testing compilation
cp userspace/apps/hello/hello.c sysroot/hello.c

# Include Headers
echo "Copying Headers..."
cp userspace/libc/*.h sysroot/usr/include/
cp userspace/libc/sys/*.h sysroot/usr/include/sys/

# TCC Includes (stdarg.h etc from TCC source)
# TCC builds its own include dir usually
if [ -d "../tcc_build/include" ]; then
    cp -r ../tcc_build/include/* sysroot/usr/include/
fi

# LibC Objects
echo "Copying Libraries..."
cp userspace/libc/*.o sysroot/usr/lib/
# Rename for standard expectations if needed
# TCC might look for 'libc.a' or similar. 
# Mithl-OS relies on crt0.o + object files for now.
# We'll just provide them.

echo "Deployment structure ready in sysroot/"
