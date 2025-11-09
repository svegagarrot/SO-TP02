global loader
extern main
extern initializeKernelBinary

loader:
	call initializeKernelBinary	
	mov rsp, rax			
	call main
	
hang:
	cli
	hlt	; halt machine should kernel return
	jmp hang
