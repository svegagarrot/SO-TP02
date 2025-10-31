#include "syscalls_lib.h"
#include "keyboardDriver.h"
#include "videoDriver.h"
#include "audioDriver.h"
#include "rtc.h"
#include <stddef.h>
#include <lib.h>
#include "interrupts.h"
#include "time.h"
#include "keystate.h"
#include "process.h"
#include "scheduler.h"
#include "mm.h"
#include <string.h>
#include "semaphore.h"

#define STDIN 0
#define STDOUT 1

extern uint64_t * _getSnapshot();

uint64_t syscall_get_regs(uint64_t *dest) {
    const uint64_t *src = _getSnapshot();
    for (int i = 0; i < REGISTERS_CANT; i++)
        dest[i] = src[i];
    return 1;
}

uint64_t syscall_read(int fd, char *buffer, int count) {
    if (fd != STDIN || buffer == NULL || count <= 0){
        return 0;
    }

    uint64_t read = 0;
    char c;

    while (read < count) {
    c = keyboard_read_getchar();
    if (c == 0) break;
    buffer[read++] = c;
    if (c == '\n') break; 
}

    return read;
}


uint64_t syscall_write(int fd, const char * buffer, int count) {
    if (fd != STDOUT) {
        return 0;
    }
    
    for (int i = 0; i < count; i++) {
        video_putChar(buffer[i], FOREGROUND_COLOR, BACKGROUND_COLOR);
    }
    
    return count;
}


uint64_t syscall_getTime(uint64_t reg) {
    uint8_t t = getTime((uint8_t)reg);
    return (uint64_t)t;
}

uint64_t syscall_clearScreen() {
    video_clearScreen();
    return 1;
}

uint64_t syscall_beep(int frequency, int duration) {
    audio_play(frequency);
    syscall_sleep(duration);
    audio_stop();
    return 1;
}

uint64_t syscall_sleep(int duration) {

    const uint32_t HZ = 18;
    uint64_t start     = ticks_elapsed();
    uint64_t wait_tics = (duration * HZ + 999) / 1000;
    uint64_t target    = start + wait_tics;


    while (ticks_elapsed() < target) {
        _hlt();
    }
    return 0;
}

uint64_t syscall_setFontScale(int scale) {
    if (scale < 1 || scale > 5) {
        return 0; 
    }
    
    setFontScale(scale);
    return 1; 
}

uint64_t syscall_video_putPixel(uint64_t x, uint64_t y, uint64_t color, uint64_t unused1, uint64_t unused2) {
    video_putPixel((uint32_t)color, x, y);
    return 0;
}

uint64_t syscall_video_putChar(uint64_t c, uint64_t fg, uint64_t bg, uint64_t unused1, uint64_t unused2) {
    video_putChar((char)c, fg, bg);
    return 0;
}

uint64_t syscall_video_clearScreenColor(uint64_t color, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    video_clearScreenColor((uint32_t)color);
    return 0;
}

uint64_t syscall_video_putCharXY(uint64_t c, uint64_t x, uint64_t y, uint64_t fg, uint64_t bg) {
    video_putCharXY((char)c, (int)x, (int)y, (uint32_t)fg, (uint32_t)bg);
    return 0;
}

uint64_t syscall_is_key_pressed(uint64_t scancode) {
    return (uint64_t)is_key_pressed((uint8_t)scancode);
}

uint64_t syscall_shutdown(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    outw(0x604, 0x2000);
    
    _cli();
    
    while(1) {
        _hlt();
    }
    
    return 0; 
}

uint64_t syscall_get_screen_dimensions(uint64_t *width, uint64_t *height) {
    if (width == NULL || height == NULL) {
        return 0;
    }
    *width = video_get_width();
    *height = video_get_height();
    return 1;
}
uint64_t syscall_malloc(uint64_t size, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (size == 0) {
        return 0;
    }
    void *ptr = mm_alloc(size);
    return (uint64_t)ptr;
}

uint64_t syscall_free(uint64_t address, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (address == 0) {
        return 0;
    }
    mm_free((void *)address);
    return 1;
}

uint64_t syscall_meminfo(uint64_t user_addr, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (user_addr == 0) {
        return 0;
    }
    mm_stats_t stats;
    mm_get_stats(&stats);
    memcpy((void *)user_addr, &stats, sizeof(stats));
    return 1;
}

uint64_t syscall_create_process(char *name, void *function, char *argv[]) {
    process_t *p = process_create(
        name ? name : "user_process",
        (process_entry_point_t)function,
        (void *)argv,
        NULL
    );
    if (!p) return 0;
    scheduler_add_process(p);
    return p->pid;
}

uint64_t syscall_kill(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return scheduler_kill_by_pid(pid);
}

uint64_t syscall_block(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return scheduler_block_by_pid(pid);
}

uint64_t syscall_unblock(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return scheduler_unblock_by_pid(pid);
}

uint64_t syscall_get_type_of_mm(uint64_t user_addr, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (user_addr == 0) return 0;
    const char *name = mm_get_manager_name();
    size_t len = strlen(name) + 1;
    memcpy((void *)user_addr, name, len);
    return 1;
}

uint64_t syscall_getpid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    process_t *p = scheduler_current_process();
    return p ? p->pid : 0;
}

uint64_t syscall_set_priority(uint64_t pid, uint64_t newPrio, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return (uint64_t)scheduler_set_priority(pid, (uint8_t)newPrio);
}

uint64_t syscall_wait(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if ((int64_t)pid <= 0) return 0;
    process_t *target = scheduler_find_by_pid(pid);
    if (!target) return 0;

    if (target->state == PROCESS_STATE_FINISHED) {
        return 1;
    }

    process_t *me = scheduler_current_process();
    if (!me || me == target || me == NULL) return 0;
    me->waiting_on_pid = pid;
    me->waiter_next = target->waiters_head;
    target->waiters_head = me;
    scheduler_block_current();
    return 1;
}

uint64_t syscall_sem_create(int initial) {
    if (initial < 0) return 0;
    uint64_t id = sem_alloc(initial);
    return id;
}

uint64_t syscall_sem_open(uint64_t sem_id) {
    return sem_open_by_id(sem_id);
}

uint64_t syscall_sem_close(uint64_t sem_id) {
    return sem_close_by_id(sem_id);
}

uint64_t syscall_sem_wait(uint64_t sem_id) {
    return sem_wait_by_id(sem_id);
}

uint64_t syscall_sem_signal(uint64_t sem_id) {
    return sem_signal_by_id(sem_id);
}

uint64_t syscall_sem_set(uint64_t sem_id, int newval) {
    return sem_set_by_id(sem_id, newval);
}

uint64_t syscall_sem_get(uint64_t sem_id) {
    int val = 0;
    if (!sem_get_value_by_id(sem_id, &val)) return (uint64_t)-1;
    return (uint64_t)val;
}

uint64_t syscall_list_processes(uint64_t user_addr, uint64_t max_count, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (user_addr == 0 || max_count == 0 || max_count > MAX_PROCESS_INFO) {
        return 0;
    }
    uint64_t count = scheduler_list_all_processes((process_info_t *)user_addr, max_count);
    return count;
}