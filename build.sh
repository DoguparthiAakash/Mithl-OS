#!/bin/bash

echo "Building Mythl OS with macOS-style GUI..."

# Clean previous build
echo "Cleaning previous build..."
make clean

# Build the kernel
echo "Building kernel..."
make

if [ $? -eq 0 ]; then
    echo "Build successful! Kernel created: kernel.elf"
    
    # Create ISO if requested
    if [ "$1" = "iso" ]; then
        echo "Creating bootable ISO..."
        make iso
        if [ $? -eq 0 ]; then
            echo "ISO created successfully: bootiso/Mithl.iso"
        fi
    fi
    
    echo "Build complete!"
else
    echo "Build failed!"
    exit 1
fi
