[bits 64]

; syscall_entry.asm — SYSCALL/SYSRET entry/exit stub
;
; On SYSCALL, CPU sets: RCX=user_RIP, R11=user_RFLAGS
; User passes: RAX=nr, RDI=a0, RSI=a1, RDX=a2, R10=a3, R8=a4, R9=a5

extern syscall_dispatch
global syscall_entry
global syscall_kernel_rsp

section .data
align 8
syscall_kernel_rsp: dq 0

section .bss
align 8
user_rsp_scratch: resq 1

section .text

syscall_entry:
    ; Switch to kernel stack
    mov [rel user_rsp_scratch], rsp
    mov rsp, [rel syscall_kernel_rsp]

    ; Save user context
    push qword [rel user_rsp_scratch]   ; user RSP
    push r11                             ; user RFLAGS
    push rcx                             ; user RIP

    ; Save callee-saved registers
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    sti

    ; Preserve Linux syscall ABI volatile registers
    ; The C compiler considers RDI, RSI, RDX, R8, R9, R10 volatile and will clobber them!
    push rdi
    push rsi
    push rdx
    push r10
    push r8
    push r9

    ; Shuffle registers for C call:
    ; C ABI:  RDI  RSI  RDX  RCX  R8   R9  [stack]
    ; Have:   RAX  RDI  RSI  RDX  R10  R8   R9
    
    ; Setup arguments to syscall_dispatch
    ; Wait, the arguments are currently on the stack from our push!
    ; [rsp]    = r9
    ; [rsp+8]  = r8
    ; [rsp+16] = r10
    ; [rsp+24] = rdx
    ; [rsp+32] = rsi
    ; [rsp+40] = rdi
    
    mov r9, r8   ; arg6 = a4 (R8)
    mov r8, r10  ; arg5 = a3 (R10)
    mov rcx, rdx ; arg4 = a2 (RDX)
    mov rdx, rsi ; arg3 = a1 (RSI)
    mov rsi, rdi ; arg2 = a0 (RDI)
    mov rdi, rax ; arg1 = nr (RAX)
    push r11
    mov r12, [rsp+8] ; Get original R9 (which was a5)
    push r12         ; Push a5 so it's [RSP]
    
    call syscall_dispatch

    add rsp, 16      ; pop a5 and dummy r11

    ; Restore user volatile registers
    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi

    cli

    ; Restore callee-saved
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ; Restore user context
    pop rcx         ; user RIP
    pop r11         ; user RFLAGS
    pop rsp         ; user RSP

    o64 sysret
