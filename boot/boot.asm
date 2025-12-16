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

SECTION .text
global start
extern kmain

start:
    ; Upon entry from GRUB (32-bit), EAX = multiboot magic, EBX = multiboot info ptr

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
