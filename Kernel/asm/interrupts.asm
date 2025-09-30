GLOBAL _cli
GLOBAL _sti
GLOBAL picMasterMask
GLOBAL picSlaveMask
GLOBAL haltcpu
GLOBAL _hlt

GLOBAL _irq00Handler
GLOBAL _irq01Handler
GLOBAL _irq02Handler
GLOBAL _irq03Handler
GLOBAL _irq04Handler
GLOBAL _irq05Handler
GLOBAL _irq80Handler
GLOBAL _exception0Handler
GLOBAL _exception6Handler
GLOBAL request_snapshot
GLOBAL _getSnapshot

EXTERN irqDispatcher
EXTERN exceptionDispatcher
EXTERN syscallDispatcher
EXTERN getStackBase
EXTERN timer_handler

SECTION .text

%macro pushState 0
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popState 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

%macro irqHandlerMaster 1
	pushState

	mov rdi, %1 ; pasaje de parametro
	call irqDispatcher

	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	popState
	iretq
%endmacro

_hlt:
	sti
	hlt
	ret

_cli:
	cli
	ret

_sti:
	sti
	ret

picMasterMask:
	push rbp
    mov rbp, rsp
    mov ax, di
    out	21h,al
    pop rbp
    retn

picSlaveMask:
	push    rbp
    mov     rbp, rsp
    mov     ax, di
    out	0A1h,al
    pop     rbp
    retn


;8254 Timer (Timer Tick)
_irq00Handler:
	pushState

    call timer_handler ;ticks++

    mov rdi, rsp
    call schedule
    mov rsp, rax

    ;send EOI
    mov al, 20h
    out 20h, al

    popState
    iretq

;Keyboard
_irq01Handler:
    pushState

    cmp byte [do_snapshot], 1
    jne .no_snapshot

    mov rsi, rsp             
    mov rdi, snapshot_buffer  

    mov   rax, [rsi + 14*8]
    mov   [rdi +  0*8], rax

    mov   rax, [rsi + 13*8]
    mov   [rdi +  1*8], rax

    mov   rax, [rsi + 12*8]
    mov   [rdi +  2*8], rax

    mov   rax, [rsi + 11*8]
    mov   [rdi +  3*8], rax

    mov   rax, [rsi + 10*8]
    mov   [rdi +  4*8], rax

    mov   rax, [rsi +  9*8]
    mov   [rdi +  5*8], rax

    mov   rax, [rsi +  8*8]
    mov   [rdi +  6*8], rax

    mov   rax, [rsi +  7*8]
    mov   [rdi +  7*8], rax

    mov   rax, [rsi +  6*8]
    mov   [rdi +  8*8], rax

    mov   rax, [rsi +  5*8]
    mov   [rdi +  9*8], rax

    mov   rax, [rsi +  4*8]
    mov   [rdi + 10*8], rax

    mov   rax, [rsi +  3*8]
    mov   [rdi + 11*8], rax

    mov   rax, [rsi +  2*8]
    mov   [rdi + 12*8], rax

    mov   rax, [rsi +  1*8]
    mov   [rdi + 13*8], rax

    mov   rax, [rsi +  0*8]
    mov   [rdi + 14*8], rax

    mov rax, [rsp + 15*8]  
    mov [snapshot_buffer + 15*8], rax   ; RIP
    mov rax, [rsp + 18*8]  
    mov [snapshot_buffer + 16*8], rax   ; RSP
    mov rax, [rsp + 17*8]  
    mov [snapshot_buffer + 17*8], rax   ; RFLAGS

    mov byte [do_snapshot], 0

.no_snapshot:
    mov rdi, 1 
    call irqDispatcher

    mov al, 20h
    out 20h, al
    popState
    iretq

_getSnapshot:
    mov rax, snapshot_buffer
    ret

;Cascade pic never called
_irq02Handler:
	irqHandlerMaster 2

;Serial Port 2 and 4
_irq03Handler:
	irqHandlerMaster 3

;Serial Port 1 and 3
_irq04Handler:
	irqHandlerMaster 4

;USB
_irq05Handler:
	irqHandlerMaster 5

;Syscall
_irq80Handler:
    push rbx
    push rcx
    push rdx
    push rbp
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov   r9, rax
    call  syscallDispatcher  

    pop   r15
    pop   r14
    pop   r13
    pop   r12
    pop   r11
    pop   r10
    pop   r9
    pop   r8
    pop   rsi
    pop   rdi
    pop   rbp
    pop   rdx
    pop   rcx
    pop   rbx

    iretq

%macro exceptionHandler 1

	pushState
	mov [exception_regs + 8*0 ], rax
	mov [exception_regs + 8*1 ], rbx
	mov [exception_regs + 8*2 ], rcx
	mov [exception_regs + 8*3 ], rdx
	mov [exception_regs + 8*4 ], rsi
	mov [exception_regs + 8*5 ], rdi
	mov [exception_regs + 8*6 ], rbp
    mov rax, [rsp + 18 * 8]
	mov [exception_regs + 8*7 ], rax	
	mov [exception_regs + 8*8 ], r8
	mov [exception_regs + 8*9 ], r9
	mov [exception_regs + 8*10], r10
	mov [exception_regs + 8*11], r11
	mov [exception_regs + 8*12], r12
	mov [exception_regs + 8*13], r13
	mov [exception_regs + 8*14], r14
	mov [exception_regs + 8*15], r15
	mov rax, [rsp+15*8]                   
	mov [exception_regs + 8*16], rax
	mov rax, [rsp+17*8]                    
	mov [exception_regs + 8*17], rax

	mov rdi, %1                            
	mov rsi, exception_regs
	call exceptionDispatcher

	popState                             
    call getStackBase                     
	mov [rsp+24], rax 
    mov rax, userland
    mov [rsp], rax   

    sti
    iretq

%endmacro

;Zero division exception
_exception0Handler:
	exceptionHandler 0
	jmp haltcpu

;InvalidOp exception
_exception6Handler:	
	exceptionHandler 6
	jmp haltcpu


haltcpu:
	cli
	hlt
	ret

request_snapshot:
    mov byte [do_snapshot], 1
    ret

SECTION .bss
	aux resq 1
	exception_regs resq 18
	snapshot_buffer resq 18
	do_snapshot resb 1

SECTION .rodata
	userland equ 0x400000