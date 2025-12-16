; kernel/gdt_flush.asm

global gdt_flush

gdt_flush:
    mov eax, [esp+4] ; Get pointer to GDT
    lgdt [eax]       ; Load GDT

    mov ax, 0x10     ; Offset 0x10 is our Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.flush  ; Far jump to reload CS to 0x08 (Code Segment)
.flush:
    ret
