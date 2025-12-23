#!/bin/bash
set -e

# Configuration
INITRD_DIR="build/initrd"
OUTPUT_IMG="boot/initramfs.cpio.gz"
KERNEL_MOD="modules/mithl_core/mithl_core.ko"
COMPOSITOR="userspace/gui/compositor"

echo "[Mithl-OS] Building Initramfs..."

# 1. Clean and Create Directory Structure
rm -rf "$INITRD_DIR"
mkdir -p "$INITRD_DIR"/{bin,dev,proc,sys,modules,etc}
mkdir -p boot

# 2. Copy Components
# Copy Kernel Module
if [ -f "$KERNEL_MOD" ]; then
    cp "$KERNEL_MOD" "$INITRD_DIR/modules/"
else
    echo "Error: $KERNEL_MOD not found. Run 'make core' first."
    exit 1
fi

# Copy Compositor (GUI)
if [ -f "$COMPOSITOR" ]; then
    cp "$COMPOSITOR" "$INITRD_DIR/bin/compositor"
else
    echo "Error: $COMPOSITOR not found. Run 'make gui' first."
    exit 1
fi

# Copy Busybox
BUSYBOX=$(which busybox)
if [ -f "$BUSYBOX" ]; then
    cp "$BUSYBOX" "$INITRD_DIR/bin/busybox"
    chmod +x "$INITRD_DIR/bin/busybox"
    # Create essential symlinks
    ln -s busybox "$INITRD_DIR/bin/sh"
    ln -s busybox "$INITRD_DIR/bin/mount"
    ln -s busybox "$INITRD_DIR/bin/echo"
    ln -s busybox "$INITRD_DIR/bin/insmod"
    ln -s busybox "$INITRD_DIR/bin/ls"
else
    echo "Error: busybox not found."
    exit 1
fi

# 3. Create Init Script
cat > "$INITRD_DIR/init" << 'EOF'
#!/bin/sh

# Mount essential filesystems
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

echo "--------------------------------"
echo "   Welcome to Mithl-OS Hybrid   "
echo "--------------------------------"

# Load Semantic Core Driver
echo "[init] Loading Mithl Core..."
insmod /modules/mithl_core.ko

# Check if device node exists (mdev/udev usually handles this, but we force it for now if needed)
# The module should create /dev/mithl automatically if devtmpfs is working.
echo "[init] Devices in /dev:"
ls /dev

# Launch GUI
echo "[init] Starting Mithl GUI..."
if [ -e /dev/fb0 ]; then
    /bin/compositor
else
    echo "Error: /dev/fb0 missing. Attempting to create node..."
    mknod /dev/fb0 c 29 0
    /bin/compositor
fi

# Fallback shell if GUI exits
echo "[init] GUI exited. Dropping to shell..."
exec /bin/sh
EOF

chmod +x "$INITRD_DIR/init"

# 4. Pack Initramfs
cd "$INITRD_DIR"
find . -print0 | cpio --null -ov --format=newc | gzip > "../../$OUTPUT_IMG"
cd ../..

echo "[Mithl-OS] Initramfs created at $OUTPUT_IMG"
