section .text

GLOBAL sys_read
GLOBAL sys_write
GLOBAL sys_getTime
GLOBAL sys_clearScreen
GLOBAL sys_beep
GLOBAL sys_sleep
GLOBAL sys_setFontScale
GLOBAL sys_video_putPixel
GLOBAL sys_video_putChar
GLOBAL sys_video_clearScreenColor
GLOBAL sys_video_putCharXY
GLOBAL sys_regs
GLOBAL sys_is_key_pressed
GLOBAL sys_shutdown
GLOBAL sys_screenDims
GLOBAL sys_malloc
GLOBAL sys_free
GLOBAL sys_meminfo
GLOBAL sys_create_process
GLOBAL sys_kill
GLOBAL sys_block
GLOBAL sys_unblock
GLOBAL sys_get_type_of_mm
GLOBAL sys_getpid
GLOBAL sys_set_priority
GLOBAL sys_wait


sys_read:
    push rbp
    mov rbp, rsp
    mov rax, 0
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_write:
    push rbp
    mov rbp, rsp
    mov rax, 1
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_getTime:
    push rbp
    mov rbp, rsp
    mov rax, 2   
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_clearScreen:
    push rbp
    mov rbp, rsp
    mov rax, 3     
    int 0x80
    mov rsp, rbp
    pop rbp 
    ret

sys_beep:
    push rbp
    mov rbp, rsp
    mov rax, 4
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_sleep:
    push rbp
    mov rbp, rsp
    mov rax, 5
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_setFontScale:
    push rbp
    mov rbp, rsp
    mov rax, 6
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_video_clearScreenColor:
    push rbp
    mov rbp, rsp
    mov rax, 7       
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_video_putPixel:
    push rbp
    mov rbp, rsp
    mov rax, 8        
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_video_putChar:
    push rbp
    mov rbp, rsp
    mov rax, 9       
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_video_putCharXY:
    push rbp
    mov rbp, rsp
    mov rax, 10
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_regs:
    push rbp
    mov rbp, rsp
    mov rax, 11
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_is_key_pressed:
    push rbp
    mov rbp, rsp
    mov rax, 12
    mov rdi, rdi   
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_shutdown:
    push rbp
    mov rbp, rsp
    mov rax, 13    
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_screenDims:
    push rbp
    mov rbp, rsp
    mov rax, 14    
    int 0x80
    mov rsp, rbp
    pop rbp
    ret
sys_malloc:
    push rbp
    mov rbp, rsp
    mov rax, 15
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_free:
    push rbp
    mov rbp, rsp
    mov rax, 16
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_meminfo:
    push rbp
    mov rbp, rsp
    mov rax, 17
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_create_process:
    push rbp
    mov rbp, rsp
    mov rax, 18
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_kill:
    push rbp
    mov rbp, rsp
    mov rax, 19
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_block:
    push rbp
    mov rbp, rsp
    mov rax, 20
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_unblock:
    push rbp
    mov rbp, rsp
    mov rax, 21
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_get_type_of_mm:
    push rbp
    mov rbp, rsp
    mov rax, 22
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_getpid:
    push rbp
    mov rbp, rsp
    mov rax, 23
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_set_priority:
    push rbp
    mov rbp, rsp
    mov rax, 24
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_wait:
    push rbp
    mov rbp, rsp
    mov rax, 25
    int 0x80
    mov rsp, rbp
    pop rbp
    ret