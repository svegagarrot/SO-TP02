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

GLOBAL process_start
GLOBAL setup_process_context
GLOBAL callTimerTick

EXTERN scheduler_finish_current
EXTERN schedule

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

callTimerTick:
	int 20h
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

    mov rdi, 1 
    call irqDispatcher

    mov al, 20h
    out 20h, al
    popState
    iretq

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

; process_start: llama entry (en r8) con rdi=arg y luego marca terminado
process_start:
    and     rsp, -16
    sub     rsp, 8   

    call    r8               ; entry(arg)

    add     rsp, 8

    ; terminar proceso
    call    scheduler_finish_current

    ; No continuar ejecutando código de abajo (setup_process_context).
    ; Mantener CPU esperando próximo tick para que el scheduler saque este proceso.
.finished_loop:
    sti
    hlt
    jmp     .finished_loop

; setup_process_context(rdi=stack_top, rsi=entry_point, rdx=arg) -> rax=rsp_inicial
setup_process_context:
    push rbp
    mov  rbp, rsp

    ; Guardar args antes de tocar registros
    ; rdi=stack_top, rsi=entry_point, rdx=arg
    mov  r14, rdi         ; stack_top
    mov  r13, rsi         ; entry_point
    mov  r12, rdx         ; arg

    ; Preparar RSP alineado
    mov  rsp, r14
    and  rsp, -16

    push qword 0                    ; SS
    push r14                        ; RSP (stack vacío del proceso)
    push qword 0x0000000000000202   ; RFLAGS (IF=1)
    push qword 0x0000000000000008   ; CS = 0x08 (kernel code)
    push qword process_start        ; RIP = process_start

    push qword 0          ; rax
    push qword 0          ; rbx
    push qword 0          ; rcx
    push qword 0          ; rdx
    push r14              ; rbp = stack_top
    push r12              ; rdi = arg
    push qword 0          ; rsi = 0
    push r13              ; r8  = entry_point
    push qword 0          ; r9
    push qword 0          ; r10
    push qword 0          ; r11
    push qword 0          ; r12
    push qword 0          ; r13
    push qword 0          ; r14
    push qword 0          ; r15

    ; RAX = RSP inicial para el scheduler
    mov  rax, rsp
    mov  rsp, rbp
    pop  rbp
    ret

SECTION .bss
	aux resq 1
	exception_regs resq 18

SECTION .rodata
	userland equ 0x400000
