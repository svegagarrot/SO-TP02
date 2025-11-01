#include "syscalls_lib.h"

typedef uint64_t (*SyscallHandler)(uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t r10, uint64_t r8);

static SyscallHandler syscallHandlers[] = {
    (SyscallHandler)syscall_read,   
    (SyscallHandler)syscall_write,  
    (SyscallHandler)syscall_getTime,
    (SyscallHandler)syscall_clearScreen, 
    (SyscallHandler)syscall_beep, 
    (SyscallHandler)syscall_sleep,
    (SyscallHandler)syscall_setFontScale,
    (SyscallHandler)syscall_video_clearScreenColor, 
    (SyscallHandler)syscall_video_putPixel,
    (SyscallHandler)syscall_video_putChar, 
    (SyscallHandler)syscall_video_putCharXY, 
    (SyscallHandler)syscall_get_regs,
    (SyscallHandler)syscall_is_key_pressed,
    (SyscallHandler)syscall_shutdown,
    (SyscallHandler)syscall_get_screen_dimensions,
    (SyscallHandler)syscall_malloc,
    (SyscallHandler)syscall_free,
    (SyscallHandler)syscall_meminfo,
    (SyscallHandler)syscall_create_process,
    (SyscallHandler)syscall_kill,
    (SyscallHandler)syscall_block,
    (SyscallHandler)syscall_unblock,
    (SyscallHandler)syscall_get_type_of_mm,
    (SyscallHandler)syscall_getpid,
    (SyscallHandler)syscall_set_priority,
    (SyscallHandler)syscall_wait,
    (SyscallHandler)syscall_sem_create,
    (SyscallHandler)syscall_sem_open,
    (SyscallHandler)syscall_sem_close,
    (SyscallHandler)syscall_sem_wait,
    (SyscallHandler)syscall_sem_signal,
    (SyscallHandler)syscall_sem_set,
    (SyscallHandler)syscall_sem_get,
    (SyscallHandler)syscall_list_processes,
    (SyscallHandler)syscall_yield,
};

#define SYSCALLS_COUNT (sizeof(syscallHandlers) / sizeof(syscallHandlers[0]))

uint64_t syscallDispatcher(uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t r10, uint64_t r8, uint64_t rax) {
    if (rax >= SYSCALLS_COUNT) {
        return 0;
    }
    return syscallHandlers[rax](rdi, rsi, rdx, r10, r8);
}
