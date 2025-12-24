; Linux i386 Hello World
; Uses int 0x80 directly.
; This tests if Mithl-OS can run standard Linux static binaries.

section .text
global _start
_start:
    ; sys_write(1, msg, len)
    mov eax, 4      ; SYS_WRITE (4)
    mov ebx, 1      ; STDOUT (1)
    mov ecx, msg
    mov edx, len
    int 0x80

    ; sys_exit(0)
    mov eax, 1      ; SYS_EXIT (1)
    mov ebx, 0      ; Status 0
    int 0x80

section .data
msg db "Hello from Native Linux Compatibility Layer!", 0xa
len equ $ - msg
