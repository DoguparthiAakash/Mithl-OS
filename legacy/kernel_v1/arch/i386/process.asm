[BITS 32]
global switch_task

; void switch_task(uint32_t *next_stack, uint32_t *current_stack)
switch_task:
    ; 1. Save Current Context
    push ebp
    mov ebp, esp
    
    ; Save callee-saved registers (C ABI)
    push ebx
    push esi
    push edi
    
    ; Save flags
    pushf
    
    ; 2. Switch Stacks
    ; Argument 2: current_stack pointer address (uint32_t **)
    ; We need to save ESP into *current_stack
    mov eax, [ebp + 12] 
    mov [eax], esp
    
    ; Argument 1: next_stack pointer address (uint32_t *)
    ; We need to load ESP from next_stack
    mov eax, [ebp + 8]
    mov esp, [eax]    ; Dereference: esp = *next_stack
    
    ; 3. Restore Next Context
    popf
    
    pop edi
    pop esi
    pop ebx
    
    pop ebp
    
    ret
