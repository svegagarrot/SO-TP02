#ifndef SYSCALLS_LIB_H
#define SYSCALLS_LIB_H

#include <stdint.h>
#include "mm.h"

uint64_t syscall_read(int fd, char * buffer, int count);
uint64_t syscall_write(int fd, const char * buffer, int count);
uint64_t syscall_clearScreen();
uint64_t syscall_sleep(int duration);
uint64_t syscall_video_putChar(uint64_t c, uint64_t fg, uint64_t bg, uint64_t unused1, uint64_t unused2);
uint64_t syscall_shutdown(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5);
uint64_t syscall_malloc(uint64_t size, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_free(uint64_t address, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_meminfo(uint64_t user_addr, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_get_screen_dimensions(uint64_t *width, uint64_t *height);
extern void outw(uint16_t port, uint16_t value);
uint64_t syscall_create_process(char *name, void *function, char *argv[], uint64_t priority, uint64_t is_foreground_and_pipes);
uint64_t syscall_kill(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_block(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_unblock(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_get_type_of_mm(uint64_t user_addr, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);

// Nuevas syscalls de procesos para test_prio
uint64_t syscall_getpid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5);
uint64_t syscall_set_priority(uint64_t pid, uint64_t new_priority, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_wait(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);

// Semaphore syscalls
uint64_t syscall_sem_create(int initial);
uint64_t syscall_sem_open(uint64_t sem_id);
uint64_t syscall_sem_close(uint64_t sem_id);
uint64_t syscall_sem_wait(uint64_t sem_id);
uint64_t syscall_sem_signal(uint64_t sem_id);
uint64_t syscall_sem_set(uint64_t sem_id, int newval);
uint64_t syscall_sem_get(uint64_t sem_id);
uint64_t syscall_list_processes(uint64_t user_addr, uint64_t max_count, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_yield(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5);

// Pipe syscalls
uint64_t syscall_pipe_create(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5);
uint64_t syscall_pipe_open(uint64_t pipe_id, uint64_t is_writer, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_pipe_close(uint64_t pipe_id, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_pipe_dup(uint64_t pipe_id, uint64_t fd, uint64_t mode, uint64_t unused1, uint64_t unused2);
uint64_t syscall_pipe_release_fd(uint64_t fd, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_get_foreground_pid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5);

#endif
