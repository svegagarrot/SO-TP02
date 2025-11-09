GLOBAL cpuVendor
GLOBAL getScanCode
GLOBAL inb
GLOBAL outb
GLOBAL outw
GLOBAL process_user_entry
GLOBAL acquire
GLOBAL release

section .text
	
cpuVendor:
	push rbp
	mov rbp, rsp

	push rbx

	mov rax, 0
	cpuid

	mov [rdi], ebx
	mov [rdi + 4], edx
	mov [rdi + 8], ecx
	mov byte [rdi+13], 0
	mov rax, rdi

	pop rbx

	mov rsp, rbp
	pop rbp
	ret

getScanCode:
    xor rax, rax   
    in al, 0x60  
    ret          

inb:
    push rbp
    mov rbp, rsp
    in al, dx
    mov rsp, rbp
    pop rbp
    ret

outb:
    push rbp
    mov rbp, rsp
    
    mov dx, di
    mov al, sil
    out dx, al    

    mov rsp, rbp
    pop rbp
    ret

outw:
    push rbp
    mov rbp, rsp
    
    mov rdx, rdi    
    mov rax, rsi    
    out dx, ax
    
    mov rsp, rbp
    pop rbp
    ret

;process_user_entry cambia el stack al del usuario, 
;llama a la función de entrada del proceso con un argumento, 
;y al terminar, restaura el stack original del kernel.
process_user_entry:
    ; rdi = user stack top, rsi = entry point, rdx = argument
    mov r8, rsp          
    mov rsp, rdi
    and rsp, -16          
    push r8                
    mov rdi, rdx          
    call rsi            
    pop r8               
    mov rsp, r8           
    ret

acquire:
    mov al, 1
    xchg al, [rdi]
    cmp al, 0
    jne acquire
    ret

release:
    mov byte [rdi], 0
    ret