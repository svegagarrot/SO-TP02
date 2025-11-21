section .text

GLOBAL sys_read
GLOBAL sys_write
GLOBAL sys_clearScreen
GLOBAL sys_sleep
GLOBAL sys_video_putChar
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
GLOBAL sys_sem_create
GLOBAL sys_sem_open
GLOBAL sys_sem_close
GLOBAL sys_sem_wait
GLOBAL sys_sem_signal
GLOBAL sys_sem_set
GLOBAL sys_sem_get
GLOBAL sys_list_processes
GLOBAL sys_yield
GLOBAL sys_pipe_create
GLOBAL sys_pipe_open
GLOBAL sys_pipe_close
GLOBAL sys_pipe_dup
GLOBAL sys_pipe_release_fd
GLOBAL sys_get_foreground_pid


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

sys_clearScreen:
    push rbp
    mov rbp, rsp
    mov rax, 2     
    int 0x80
    mov rsp, rbp
    pop rbp 
    ret

sys_sleep:
    push rbp
    mov rbp, rsp
    mov rax, 3
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_video_putChar:
    push rbp
    mov rbp, rsp
    mov rax, 4
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_shutdown:
    push rbp
    mov rbp, rsp
    mov rax, 5    
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_screenDims:
    push rbp
    mov rbp, rsp
    mov rax, 6    
    int 0x80
    mov rsp, rbp
    pop rbp
    ret
sys_malloc:
    push rbp
    mov rbp, rsp
    mov rax, 7
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_free:
    push rbp
    mov rbp, rsp
    mov rax, 8
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_meminfo:
    push rbp
    mov rbp, rsp
    mov rax, 9
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_create_process:
    push rbp
    mov rbp, rsp
    mov rax, 10
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_kill:
    push rbp
    mov rbp, rsp
    mov rax, 11
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_block:
    push rbp
    mov rbp, rsp
    mov rax, 12
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_unblock:
    push rbp
    mov rbp, rsp
    mov rax, 13
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_get_type_of_mm:
    push rbp
    mov rbp, rsp
    mov rax, 14
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_getpid:
    push rbp
    mov rbp, rsp
    mov rax, 15
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_set_priority:
    push rbp
    mov rbp, rsp
    mov rax, 16
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_wait:
    push rbp
    mov rbp, rsp
    mov rax, 17
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_sem_create:
    push rbp
    mov rbp, rsp
    mov rax, 18
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_sem_open:
    push rbp
    mov rbp, rsp
    mov rax, 19
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_sem_close:
    push rbp
    mov rbp, rsp
    mov rax, 20
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_sem_wait:
    push rbp
    mov rbp, rsp
    mov rax, 21
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_sem_signal:
    push rbp
    mov rbp, rsp
    mov rax, 22
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_sem_set:
    push rbp
    mov rbp, rsp
    mov rax, 23
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_sem_get:
    push rbp
    mov rbp, rsp
    mov rax, 24
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_list_processes:
    push rbp
    mov rbp, rsp
    mov rax, 25
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_yield:
    push rbp
    mov rbp, rsp
    mov rax, 26
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_pipe_create:
    push rbp
    mov rbp, rsp
    mov rax, 27
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_pipe_open:
    push rbp
    mov rbp, rsp
    mov rax, 28
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_pipe_close:
    push rbp
    mov rbp, rsp
    mov rax, 29
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_pipe_dup:
    push rbp
    mov rbp, rsp
    mov rax, 30
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_pipe_release_fd:
    push rbp
    mov rbp, rsp
    mov rax, 31
    int 0x80
    mov rsp, rbp
    pop rbp
    ret

sys_get_foreground_pid:
    push rbp
    mov rbp, rsp
    mov rax, 32
    int 0x80
    mov rsp, rbp
    pop rbp
    ret
