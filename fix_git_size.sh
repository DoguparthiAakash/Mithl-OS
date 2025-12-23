#!/bin/bash

echo " Cleaning up heavy files from Git Index..."

# Remove known large extensions from tracking (cached only, keeping files on disk)
git rm -r --cached *.img 2>/dev/null
git rm -r --cached *.iso 2>/dev/null
git rm -r --cached legacy/*.img 2>/dev/null
git rm -r --cached legacy/*.iso 2>/dev/null
git rm -r --cached DOOM1.WAD 2>/dev/null
git rm -r --cached kernel.elf 2>/dev/null

echo "Large files removed from index. They are now ignored by .gitignore."
echo "You can now commit and push."
