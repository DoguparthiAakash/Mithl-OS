global syscall_3
global syscall_0
global syscall_1
global syscall_2
global syscall_5

section .text

; int syscall_0(int num);
syscall_0:
    push ebp
    mov ebp, esp
    push ebx
    
    mov eax, [ebp+8] ; num
    int 0x80
    
    pop ebx
    leave
    ret

; int syscall_1(int num, int arg1);
syscall_1:
    push ebp
    mov ebp, esp
    push ebx
    
    mov eax, [ebp+8] ; num
    mov ebx, [ebp+12] ; arg1
    int 0x80
    
    pop ebx
    leave
    leave
    ret

; int syscall_2(int num, int arg1, int arg2);
syscall_2:
    push ebp
    mov ebp, esp
    push ebx
    
    mov eax, [ebp+8] ; num
    mov ebx, [ebp+12] ; arg1
    mov ecx, [ebp+16] ; arg2
    int 0x80
    
    pop ebx
    leave
    ret

; int syscall_3(int num, int arg1, int arg2, int arg3);
syscall_3:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    
    mov eax, [ebp+8]  ; num
    mov ebx, [ebp+12] ; arg1
    mov ecx, [ebp+16] ; arg2
    mov edx, [ebp+20] ; arg3
    int 0x80
    
    pop esi
    pop ebx
    leave
    ret

; int syscall_5(int num, int arg1, int arg2, int arg3, int arg4, int arg5);
syscall_5:
    push ebp
    mov ebp, esp
    push ebx
    push esi
    push edi
    
    mov eax, [ebp+8]  ; num
    mov ebx, [ebp+12] ; arg1
    mov ecx, [ebp+16] ; arg2
    mov edx, [ebp+20] ; arg3
    mov esi, [ebp+24] ; arg4
    mov edi, [ebp+28] ; arg5
    int 0x80
    
    pop edi
    pop esi
    pop ebx
    leave
    ret
