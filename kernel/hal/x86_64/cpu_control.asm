; Hardware Abstraction Layer - CPU Control
; Advanced Assembly for critical hardware operations

[BITS 64]

section .text

; ============================================================================
; CPU Feature Detection
; ============================================================================

global cpu_has_sse
global cpu_has_avx
global cpu_has_avx2
global cpu_has_aes

; Check if SSE is supported
cpu_has_sse:
    push rbx
    mov eax, 1
    cpuid
    test edx, (1 << 25)  ; Check SSE bit
    setnz al
    movzx rax, al
    pop rbx
    ret

; Check if AVX is supported
cpu_has_avx:
    push rbx
    mov eax, 1
    cpuid
    test ecx, (1 << 28)  ; Check AVX bit
    setnz al
    movzx rax, al
    pop rbx
    ret

; Check if AVX2 is supported
cpu_has_avx2:
    push rbx
    mov eax, 7
    xor ecx, ecx
    cpuid
    test ebx, (1 << 5)   ; Check AVX2 bit
    setnz al
    movzx rax, al
    pop rbx
    ret

; Check if AES-NI is supported
cpu_has_aes:
    push rbx
    mov eax, 1
    cpuid
    test ecx, (1 << 25)  ; Check AES bit
    setnz al
    movzx rax, al
    pop rbx
    ret

; ============================================================================
; CPU Control - Enable Advanced Features
; ============================================================================

global cpu_enable_sse
global cpu_enable_avx
global cpu_enable_fpu

; Enable SSE instructions
cpu_enable_sse:
    mov rax, cr0
    and ax, 0xFFFB       ; Clear CR0.EM (bit 2)
    or ax, 0x2           ; Set CR0.MP (bit 1)
    mov cr0, rax
    
    mov rax, cr4
    or ax, 3 << 9        ; Set CR4.OSFXSR and CR4.OSXMMEXCPT
    mov cr4, rax
    ret

; Enable AVX instructions
cpu_enable_avx:
    ; First enable SSE
    call cpu_enable_sse
    
    ; Enable XSAVE
    mov rax, cr4
    or rax, (1 << 18)    ; Set CR4.OSXSAVE
    mov cr4, rax
    
    ; Enable AVX in XCR0
    xor ecx, ecx
    xgetbv               ; Load XCR0
    or eax, 7            ; Enable x87, SSE, AVX
    xsetbv
    ret

; Enable FPU
cpu_enable_fpu:
    mov rax, cr0
    and ax, 0xFFF3       ; Clear CR0.EM and CR0.TS
    or ax, 0x22          ; Set CR0.NE and CR0.MP
    mov cr0, rax
    fninit               ; Initialize FPU
    ret

; ============================================================================
; MSR (Model Specific Register) Access
; ============================================================================

global cpu_rdmsr
global cpu_wrmsr

; Read MSR
; Input: RDI = MSR number
; Output: RAX = MSR value
cpu_rdmsr:
    mov ecx, edi
    rdmsr
    shl rdx, 32
    or rax, rdx
    ret

; Write MSR
; Input: RDI = MSR number, RSI = value
cpu_wrmsr:
    mov ecx, edi
    mov eax, esi
    shr rsi, 32
    mov edx, esi
    wrmsr
    ret

; ============================================================================
; Atomic Operations
; ============================================================================

global atomic_add
global atomic_sub
global atomic_cmpxchg
global atomic_inc
global atomic_dec

; Atomic add
; Input: RDI = pointer, RSI = value
; Output: RAX = old value
atomic_add:
    mov rax, rsi
    lock xadd [rdi], rax
    ret

; Atomic subtract
; Input: RDI = pointer, RSI = value
; Output: RAX = old value
atomic_sub:
    neg rsi
    mov rax, rsi
    lock xadd [rdi], rax
    ret

; Atomic compare-and-swap
; Input: RDI = pointer, RSI = old value, RDX = new value
; Output: RAX = actual old value, ZF set if successful
atomic_cmpxchg:
    mov rax, rsi
    lock cmpxchg [rdi], rdx
    ret

; Atomic increment
; Input: RDI = pointer
atomic_inc:
    lock inc qword [rdi]
    ret

; Atomic decrement
; Input: RDI = pointer
atomic_dec:
    lock dec qword [rdi]
    ret

; ============================================================================
; Memory Barriers
; ============================================================================

global memory_barrier
global read_barrier
global write_barrier

memory_barrier:
    mfence
    ret

read_barrier:
    lfence
    ret

write_barrier:
    sfence
    ret

; ============================================================================
; SIMD Operations - Fast Memory Copy
; ============================================================================

global simd_memcpy_sse
global simd_memcpy_avx

; Fast memcpy using SSE (16 bytes at a time)
; Input: RDI = dest, RSI = src, RDX = size
simd_memcpy_sse:
    push rbx
    mov rcx, rdx
    shr rcx, 4           ; Divide by 16
    
.loop:
    test rcx, rcx
    jz .remainder
    
    movdqu xmm0, [rsi]
    movdqu [rdi], xmm0
    
    add rsi, 16
    add rdi, 16
    dec rcx
    jmp .loop
    
.remainder:
    mov rcx, rdx
    and rcx, 15          ; Remainder bytes
    rep movsb
    
    pop rbx
    ret

; Fast memcpy using AVX (32 bytes at a time)
; Input: RDI = dest, RSI = src, RDX = size
simd_memcpy_avx:
    push rbx
    mov rcx, rdx
    shr rcx, 5           ; Divide by 32
    
.loop:
    test rcx, rcx
    jz .remainder
    
    vmovdqu ymm0, [rsi]
    vmovdqu [rdi], ymm0
    
    add rsi, 32
    add rdi, 32
    dec rcx
    jmp .loop
    
.remainder:
    mov rcx, rdx
    and rcx, 31          ; Remainder bytes
    rep movsb
    
    vzeroupper           ; Clear upper AVX state
    pop rbx
    ret

; ============================================================================
; Port I/O
; ============================================================================

global outb
global outw
global outl
global inb
global inw
global inl

; Output byte to port
; Input: RDI = port, RSI = value
outb:
    mov dx, di
    mov al, sil
    out dx, al
    ret

; Output word to port
outw:
    mov dx, di
    mov ax, si
    out dx, ax
    ret

; Output dword to port
outl:
    mov dx, di
    mov eax, esi
    out dx, eax
    ret

; Input byte from port
; Input: RDI = port
; Output: RAX = value
inb:
    mov dx, di
    xor rax, rax
    in al, dx
    ret

; Input word from port
inw:
    mov dx, di
    xor rax, rax
    in ax, dx
    ret

; Input dword from port
inl:
    mov dx, di
    xor rax, rax
    in eax, dx
    ret
