[bits 64]
global switch_context

; void switch_context(struct cpu_context **old, struct cpu_context *new_ctx);
; RDI = struct cpu_context **old
; RSI = struct cpu_context *new_ctx
switch_context:
    ; The compiler CALL instruction just pushed RIP on the stack.
    ; Now push callee-saved registers so they match the cpu_context struct.
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; If old is NULL (for initial startup wrapper), don't write.
    test rdi, rdi
    jz .skip_save
    
    ; Save current RSP to *old
    mov [rdi], rsp

.skip_save:
    ; Switch to the new RSP (new_ctx)
    mov rsp, rsi

    ; Pop callee-saved registers for the new context
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ; The top of the stack is now the RIP of the new context.
    ; Execute 'ret' to pop it into the instruction pointer and jump!
    ret

extern kthread_exit
extern sched_release_lock
global kernel_thread_stub

kernel_thread_stub:
    ; switch_context has returned to this stub.
    ; If we were scheduled from an ISR or schedule(), the scheduler lock is still held.
    call sched_release_lock

    ; If we were scheduled from an ISR, interrupts are disabled.
    ; Re-enable them so this thread can be preempted.
    sti

    ; R12 = function pointer
    ; R13 = argument
    mov rdi, r13  ; move arg into RDI for System V ABI first arg
    call r12      ; call the C function

    ; if the C function returns, we must exit gracefully
    call kthread_exit
