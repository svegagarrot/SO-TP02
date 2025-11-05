#ifndef LIB_H
#define LIB_H

#define BUFFER_SIZE 256
#define stdin ((FILE*)0) 
#define REGISTERS_CANT 18

#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS 0x04
#define RTC_DAY 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09

#include <stdint.h>
#include <stddef.h>
#include <math.h>

// Definición básica de FILE para compatibilidad
typedef struct FILE FILE; 

typedef struct {
    uint64_t rax, rbx, rcx, rdx, rbp, rdi, rsi, r8, r9, r10, r11, r12, r13, r14, r15, rip, rsp, rflags;
} CPURegisters;

typedef struct {
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t free_bytes;
    uint64_t largest_free_block;
    uint64_t allocations;
    uint64_t frees;
    uint64_t failed_allocations;
} memory_info_t;

// Estados de proceso
#define PROCESS_STATE_NEW 0
#define PROCESS_STATE_READY 1
#define PROCESS_STATE_RUNNING 2
#define PROCESS_STATE_BLOCKED 3
#define PROCESS_STATE_FINISHED 4

#define PROCESS_NAME_MAX_LEN 32
#define MAX_PROCESS_INFO 64

typedef struct {
    uint64_t pid;
    char name[PROCESS_NAME_MAX_LEN + 1];
    int state;
    int priority;
    uint64_t rsp;
    uint64_t rbp;
    int foreground;
} process_info_t;

extern int current_font_scale;

int putchar(int c);
int getchar(void);
int scanf(const char *fmt, ...);
int printf(const char *fmt, ...);
size_t strlen(const char *s);
char *strncpy(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int atoi(const char *str);
char *fgets(char *s, int n, FILE *stream);
void clearScreen();
void *malloc(size_t size);
void free(void *ptr);
int memory_info(memory_info_t *info);
int sprintf(char *str, const char *fmt, ...);
int try_getchar(char *c);
void getTime(char *buffer);
void setFontScale(int scale);
void save_registers_snapshot(uint64_t *buffer);
void printHex64(uint64_t value);
void video_putPixel(int x, int y, uint32_t color);
void video_putChar(char c, uint32_t fg, uint32_t bg);
void video_clearScreenColor(uint32_t color);
void sleep(int milliseconds);
void video_putCharXY(int x, int y, char c, uint32_t fg, uint32_t bg);
void beep(int frequency, int duration);
void audiobounce();
int get_regs(uint64_t *r);
int is_key_pressed_syscall(unsigned char scancode);
void clear_key_buffer();
char *toLower(char *str);
void shutdown();
int getScreenDims(uint64_t *width, uint64_t *height);
int64_t my_create_process(char *name, void *function, char *argv[], uint64_t priority, int is_foreground);
int64_t my_kill(uint64_t pid);
int64_t my_block(uint64_t pid);
int64_t my_unblock(uint64_t pid);
int64_t my_getpid();
int64_t my_nice(uint64_t pid, uint64_t newPrio);
int64_t my_wait(int64_t pid);
int64_t my_sem_open(char *sem_id, uint64_t initialValue);
int64_t my_sem_wait(char *sem_id);
int64_t my_sem_post(char *sem_id);
int64_t my_sem_close(char *sem_id);
int64_t my_yield();
int get_type_of_mm(char *buf, int buflen);
uint64_t list_processes(process_info_t *buffer, uint64_t max_count);

// Pipe functions
uint64_t pipe_create(void);
uint64_t pipe_open(uint64_t pipe_id);
uint64_t pipe_close(uint64_t pipe_id);
uint64_t pipe_dup(uint64_t pipe_id, uint64_t fd, uint64_t mode);

#endif
