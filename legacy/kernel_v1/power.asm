[bits 32]

global asm_shutdown
global asm_reboot

section .text

; -----------------------------------------------------------------------------
; asm_shutdown
; Attempts to shut down the system using various emulator-specific ports
; and ACPI methods.
; -----------------------------------------------------------------------------
asm_shutdown:
    cli                     ; Disable interrupts

.retry:
    ; --- VirtualBox & Bochs (Best Bets) ---
    
    ; 1. Port 0x4004 (VirtualBox specific)
    mov dx, 0x4004
    mov ax, 0x3400
    out dx, ax              ; 16-bit write
    out dx, eax             ; 32-bit write (Just to be sure)
    
    mov ax, 0x2000
    out dx, ax              ; Try 'Power Off' value variant
    out dx, eax

    ; 2. Port 0xB004 (Bochs / Newer VBox)
    mov dx, 0xB004
    mov ax, 0x2000
    out dx, ax
    out dx, eax             ; 32-bit

    ; --- QEMU ---

    ; 3. Port 0x604 (QEMU Q35/Common)
    mov dx, 0x604
    mov ax, 0x2000
    out dx, ax
    out dx, eax

    ; --- Cloud Hypervisor / Others ---

    ; 4. Port 0x600
    mov dx, 0x600
    mov ax, 0x3400
    out dx, ax
    out dx, eax

    ; Loop / Halt to prevent CPU from running away if shutdown fails
    jmp .retry

    hlt
    jmp $

; -----------------------------------------------------------------------------
; asm_reboot
; Attempts to reboot the system using the PS/2 keyboard controller
; and Triple Fault.
; -----------------------------------------------------------------------------
asm_reboot:
    cli                     ; Disable interrupts

    ; Method 1: PS/2 Keyboard Controller
    ; Wait for controller to be ready (bit 1 of status reg 0x64 must be 0)
    xor ecx, ecx
.wait_kbd:
    in al, 0x64
    test al, 0x02
    jz .do_reset
    inc ecx
    cmp ecx, 10000          ; Timeout
    jl .wait_kbd

.do_reset:
    ; Pulse the reset line (output port bit 0) to low
    mov al, 0xFE
    out 0x64, al
    hlt

    ; Method 2: Triple Fault (if KBD reset fails)
    ; Load an invalid IDT (limit=0) and trigger an interrupt
    lidt [null_idt]
    int 3                   ; Trigger interrupt -> Triple Fault -> Reset

    hlt
    jmp $

section .data
    null_idt:
        dw 0    ; Limit = 0
        dd 0    ; Base = 0
