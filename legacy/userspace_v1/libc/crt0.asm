[BITS 32]

global _start
extern main
extern exit

section .text
_start:
    ; Set up stack if needed (Kernel should set ESP)
    ; Call main
    call main
    
    ; Exit with return value
    ; Exit with return value
    mov ebx, eax ; Status
    mov eax, 1   ; SYS_EXIT
    int 0x80
    hlt
    
    ; Should not reach here
    cli
    hlt
