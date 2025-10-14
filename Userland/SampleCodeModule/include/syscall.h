#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

uint64_t sys_read(int fd, char * buffer, int count);
uint64_t sys_write(int fd, const char * buffer, int count);
uint64_t sys_getTime(uint8_t reg);
uint64_t sys_clearScreen();
uint64_t sys_setFontScale(int scale);
uint64_t sys_sleep(int duration);
uint64_t sys_beep(int frequency, int duration);
uint64_t sys_video_putPixel(int x, int y, uint32_t color);
uint64_t sys_video_putChar(char c, uint32_t fg, uint32_t bg);
uint64_t sys_video_clearScreenColor(uint32_t color);
uint64_t sys_video_putCharXY(char c, int x, int y, uint32_t fg, uint32_t bg);
uint64_t sys_regs(void *user_buf);
uint64_t sys_is_key_pressed(unsigned char scancode);
uint64_t sys_shutdown();
int sys_screenDims(uint64_t *width, uint64_t *height);

void *sys_malloc(uint64_t size);
uint64_t sys_free(void *ptr);
uint64_t sys_meminfo(void *info);
uint64_t sys_create_process(char *name, void *function, char *argv[]);
uint64_t sys_kill(uint64_t pid);
uint64_t sys_block(uint64_t pid);
uint64_t sys_unblock(uint64_t pid);
uint64_t sys_get_type_of_mm(char *user_buf, int buflen);
#endif
