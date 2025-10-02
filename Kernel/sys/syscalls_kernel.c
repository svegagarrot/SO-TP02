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
    process_t *process = process_create(
        name != NULL ? name : "user_process",
        (process_entry_point_t) function,
        (void *)argv,
        0,
        PROCESS_FOREGROUND,
        PROCESS_TYPE_USER,
        NULL
    );
    if (process == NULL) {
        return 0;
    }
    scheduler_add_process(process);
    return process->pid;
}

uint64_t syscall_kill(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return scheduler_kill_by_pid(pid);
}

uint64_t syscall_block(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    process_t *current = scheduler_current_process();
    if (current == NULL || current->pid != pid) {
        return 0;
    }
    scheduler_block_current();
    return 1;
}

uint64_t syscall_unblock(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return scheduler_unblock_by_pid(pid);
}