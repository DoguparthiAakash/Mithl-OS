; boot/boot.asm - Multiboot v1 header + entry
BITS 32

; Multiboot1 Header (BIOS)
SECTION .multiboot
align 4
MULTIBOOT_PAGE_ALIGN  equ  1<<0
MULTIBOOT_MEMORY_INFO equ  1<<1
MULTIBOOT_VIDEO_MODE  equ  1<<2

MULTIBOOT_MAGIC    equ 0x1BADB002
MULTIBOOT_FLAGS    equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

; Header
dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECKSUM

; Address fields (unused if bit 16 is not set, but kept for safety/alignment)
dd 0, 0, 0, 0, 0

; Graphics request
dd 0    ; mode_type: 0 = linear graphics
dd 1024 ; width
dd 768  ; height
dd 32   ; depth

; Multiboot2 Header (UEFI/GRUB2)
SECTION .multiboot2
align 8
header_start:
    dd 0xe85250d6                ; Magic number (multiboot 2)
    dd 0                         ; Architecture 0 (protected mode i386)
    dd header_end - header_start ; Header length
    ; Checksum
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; Framebuffer Tag
    align 8
    dw 5      ; type (framebuffer)
    dw 0      ; flags
    dd 20     ; size
    dd 1024   ; width
    dd 768    ; height
    dd 32     ; depth

    ; End Tag
    align 8
    dw 0      ; type
    dw 0      ; flags
    dd 8      ; size
header_end:

SECTION .text
global start
extern kmain

start:
    ; Upon entry from GRUB (32-bit):
    ; MB1: EAX = 0x2BADB002, EBX = multiboot info ptr
    ; MB2: EAX = 0x36d76289, EBX = multiboot2 info ptr
    
    ; set up a temporary stack
    mov     esp, stack_top

    ; push arguments for kmain(uint32_t magic, uint32_t mbi)
    push    ebx         ; mbi
    push    eax         ; magic
    call    kmain

.hang:
    cli
    hlt
    jmp .hang

SECTION .bss
align 16
stack_bottom:
    resb 65536          ; 64 KiB stack
stack_top:
