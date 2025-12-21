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
    push eax
    call exit
    
    ; Should not reach here
    cli
    hlt
