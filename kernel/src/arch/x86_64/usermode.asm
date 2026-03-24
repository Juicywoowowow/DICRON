[bits 64]

; usermode.asm — Jump to ring 3 via iretq
;
; void enter_usermode(uint64_t rip, uint64_t rsp);
;   RDI = user entry point
;   RSI = user stack pointer

global enter_usermode

; GDT_USER_DS = 0x1B (index 3, RPL 3)
; GDT_USER_CS = 0x23 (index 4, RPL 3)

enter_usermode:
    push 0x1B               ; SS  = user data selector
    push rsi                ; RSP = user stack

    pushfq
    pop rax
    or rax, 0x200           ; set IF
    push rax                ; RFLAGS

    push 0x23               ; CS  = user code selector
    push rdi                ; RIP = user entry point

    iretq
