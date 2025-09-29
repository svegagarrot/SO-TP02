#ifndef SYSCALLS_LIB_H
#define SYSCALLS_LIB_H

#include <stdint.h>
#include "registers.h"

uint64_t syscall_get_regs(uint64_t *dest);
uint64_t syscall_read(int fd, char * buffer, int count);
uint64_t syscall_write(int fd, const char * buffer, int count);
uint64_t syscall_getTime(uint64_t reg);
uint64_t syscall_clearScreen();
uint64_t syscall_beep(int frequency, int duration);
uint64_t syscall_sleep(int duration);
uint64_t syscall_setFontScale(int scale);
uint64_t syscall_video_putPixel(uint64_t x, uint64_t y, uint64_t color, uint64_t unused1, uint64_t unused2);
uint64_t syscall_video_putChar(uint64_t c, uint64_t fg, uint64_t bg, uint64_t unused1, uint64_t unused2);
uint64_t syscall_video_clearScreenColor(uint64_t color, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4);
uint64_t syscall_video_putCharXY(uint64_t c, uint64_t x, uint64_t y, uint64_t fg, uint64_t bg);
uint64_t syscall_is_key_pressed(uint64_t scancode);
uint64_t syscall_shutdown(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5);
uint64_t syscall_get_screen_dimensions(uint64_t *width, uint64_t *height);
extern void outw(uint16_t port, uint16_t value);

#endif