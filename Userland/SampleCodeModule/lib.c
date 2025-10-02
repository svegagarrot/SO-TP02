#include "include/lib.h"
#include "include/syscall.h"
#include <stdarg.h>

int get_regs(uint64_t *r) {
    return sys_regs(r);
}

int putchar(int c) {
    char ch = (char)c;
    sys_write(1, &ch, 1);
    return c;
}

int getchar(void) {
    char c;
    int n;

    do {
        n = sys_read(0, &c, 1);
    } while (n == 0);

    return (int)c;
}

int is_key_pressed_syscall(unsigned char scancode) {
    return sys_is_key_pressed(scancode);
}

size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

char *fgets(char *s, int n, FILE *stream) {
    int i = 0;
    char c;

    if (n <= 0) return 0;

    while (i < n - 1) {
        int read;
        do {
            read = sys_read(0, &c, 1);  // stdin siempre es fd 0
        } while (read == 0);  

        s[i++] = c;
        if (c == '\n') break;
    }

    s[i] = '\0';
    return s;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}


int atoi(const char *str) {

    int res = 0;
    int sign = 1;
    int i = 0;
    
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
        i++;
    }
    
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }
    
    while (str[i] >= '0' && str[i] <= '9') {
        res = res * 10 + (str[i] - '0');
        i++;
    }
    
    return sign * res;
}

int scanf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char input[BUFFER_SIZE];
    if (!fgets(input, BUFFER_SIZE, stdin)) {
        va_end(args);
        return 0;
    }

    int index = 0;
    int count = 0;

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == ' ' || fmt[i] == '\t' || fmt[i] == '\n') {
            while (input[index] == ' ' || input[index] == '\t') {
                index++;
            }
            continue;
        }

        if (input[index] == '\n' || input[index] == '\0') {
            break;
        }

        if (fmt[i] == '%') {
            i++;
            if (fmt[i] == '[' && fmt[i + 1] == '^' && fmt[i + 2] == '\\' &&
                fmt[i + 3] == 'n' && fmt[i + 4] == ']') {
                i += 4;
                char *dest = va_arg(args, char *);
                int start = index;
                while (input[index] != '\0' && input[index] != '\n') {
                    index++;
                }
                int len = index - start;
                if (len > 0) {
                    for (int j = 0; j < len; j++) {
                        dest[j] = input[start + j];
                    }
                    dest[len] = '\0';
                    count++;
                }
                continue;
            }

            switch (fmt[i]) {
                case 's': {
                    char *dest = va_arg(args, char *);
                    int start = index;
                    while (input[index] != '\0' && input[index] != '\n') {
                        index++;
                    }
                    int len = index - start;
                    if (len > 0) {
                        for (int j = 0; j < len; j++) {
                            dest[j] = input[start + j];
                        }
                        dest[len] = '\0';
                        count++;
                    }
                    break;
                }
                case 'd': {
                    int *dest = va_arg(args, int *);
                    
                    while (input[index] == ' ' || input[index] == '\t') {
                        index++;
                    }
                    
                    int start = index;
                    
                    if (input[index] == '-' || input[index] == '+') {
                        index++;
                    }
                    
                    if (input[index] < '0' || input[index] > '9') {
                        break;
                    }
                    
                    while (input[index] >= '0' && input[index] <= '9') {
                        index++;
                    }
                    
                    int len = index - start;
                    if (len > 0) {
                        char temp[32];
                        if (len >= (int)sizeof(temp)) len = sizeof(temp) - 1;
                        
                        for (int j = 0; j < len; j++) {
                            temp[j] = input[start + j];
                        }
                        temp[len] = '\0';
                        
                        *dest = atoi(temp);
                        count++;
                    }
                    
                    while (input[index] == ' ' || input[index] == '\t') {
                        index++;
                    }
                    break;
                }
                case 'c': {
                    char *dest = va_arg(args, char *);
                    if (input[index] != '\0') {
                        *dest = input[index];
                        index++;
                        count++;
                    }
                    break;
                }
                default:
                    break;
            }
        } else {
            if (input[index] == fmt[i]) {
                index++;
            } else {
                break; 
            }
        }

        if (input[index] == '\n' || input[index] == '\0') {
            break;
        }
    }

    va_end(args);
    return count;
}

int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int count = 0;

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            switch (fmt[i]) {
                case 's': {
                    char *s = va_arg(args, char *);
                    while (*s) {
                        putchar(*s++);
                        count++;
                    }
                    break;
                }
                case 'd': {
                    int n = va_arg(args, int);
                    if (n == 0) {
                        putchar('0');
                        count++;
                        break;
                    }

                    if (n < 0) {
                        putchar('-');
                        n = -n;
                        count++;
                    }

                    char buf[12];
                    int i = 0;

                    while (n > 0) {
                        buf[i++] = (n % 10) + '0';
                        n /= 10;
                    }

                    while (i--) {
                        putchar(buf[i]);
                        count++;
                    }

                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    putchar(c);
                    count++;
                    break;
                }
                case 'l':
                    if (fmt[i+1]=='l' && fmt[i+2]=='x') {
                        unsigned long long v = va_arg(args, unsigned long long);
                        char buf[17];
                        buf[16] = '\0';
                        for (int j = 15; j >= 0; j--) {
                            buf[j] = "0123456789ABCDEF"[v & 0xF];
                            v >>= 4;
                        }
                        char *p = buf;
                        while (*p=='0' && *(p+1)) p++;
                        printf("%s", p);
                        count += strlen(p);
                        i += 2;  
                        break;
                    }
                default:
                    putchar('%');
                    putchar(fmt[i]);
                    count += 2;
                    break;
            }
        } else {
            putchar(fmt[i]);
            count++;
        }
    }

    va_end(args);
    return count;
}

int sprintf(char *str, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int count = 0;
    int str_index = 0;

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            switch (fmt[i]) {
                case 's': {
                    char *s = va_arg(args, char *);
                    while (*s) {
                        str[str_index++] = *s++;
                        count++;
                    }
                    break;
                }
                case 'd': {
                    int n = va_arg(args, int);
                    if (n == 0) {
                        str[str_index++] = '0';
                        count++;
                        break;
                    }

                    if (n < 0) {
                        str[str_index++] = '-';
                        n = -n;
                        count++;
                    }

                    char buf[12];
                    int buf_i = 0;

                    while (n > 0) {
                        buf[buf_i++] = (n % 10) + '0';
                        n /= 10;
                    }

                    while (buf_i--) {
                        str[str_index++] = buf[buf_i];
                        count++;
                    }
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    str[str_index++] = c;
                    count++;
                    break;
                }
                default:
                    str[str_index++] = '%';
                    str[str_index++] = fmt[i];
                    count += 2;
                    break;
            }
        } else {
            str[str_index++] = fmt[i];
            count++;
        }
    }
    
    str[str_index] = '\0';
    va_end(args);
    return count;
}

void clearScreen() {
    sys_clearScreen();
}

static uint8_t getReg(uint8_t reg) {
    return sys_getTime(reg);
}

void getTime(char *buffer) {
    uint8_t s = getReg(RTC_SECONDS);
    uint8_t m = getReg(RTC_MINUTES);
    uint8_t h = getReg(RTC_HOURS);

    buffer[0] = '0' + h/10; buffer[1] = '0' + h%10;
    buffer[2] = ':';
    buffer[3] = '0' + m/10; buffer[4] = '0' + m%10;
    buffer[5] = ':';
    buffer[6] = '0' + s/10; buffer[7] = '0' + s%10;
    buffer[8] = '\0';
}

int current_font_scale = 1;

void setFontScale(int scale) {
    current_font_scale = scale; 
    sys_setFontScale(scale);
}

void printHex64(uint64_t value) {
    char hex[17];
    for (int i = 15; i >= 0; i--) {
        int digit = value & 0xF;
        hex[i] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        value >>= 4;
    }
    hex[16] = '\0';
    printf("0x%s", hex);
}

void video_putPixel(int x, int y, uint32_t color) {
    sys_video_putPixel(x, y, color);
}

void video_putChar(char c, uint32_t fg, uint32_t bg) {
    sys_video_putChar(c, fg, bg);
}

void video_clearScreenColor(uint32_t color) {
    sys_video_clearScreenColor(color);
}

int try_getchar(char *c) {
    return sys_read(0, c, 1);
}

void sleep(int ms) {
    sys_sleep(ms);
}

void video_putCharXY(int x, int y, char c, uint32_t fg, uint32_t bg) {
    sys_video_putCharXY(c, x, y, fg, bg);
}

void beep(int frequency, int duration) {
    sys_beep(frequency, duration);
}

void clear_key_buffer() {
    char c;
    while (try_getchar(&c)) {
    }
}

char *toLower(char *str) {
    char *original = str;
    while(*str){
        if(*str >= 'A' && *str <= 'Z'){
            *str = *str + 32;
        }
        str++;
    }
    return original;
}

void shutdown() {
     sys_shutdown();
}

int getScreenDims(uint64_t *width, uint64_t *height) {
    return sys_screenDims(width, height);
}
void *malloc(size_t size) {
    return (void *)sys_malloc(size);
}

void free(void *ptr) {
    if (ptr != NULL) {
        sys_free(ptr);
    }
}

int memory_info(memory_info_t *info) {
    if (info == NULL) {
        return 0;
    }
    return (int)sys_meminfo(info);
}

int64_t my_create_process(char *name, void *function, char *argv[]) {
    if (function == NULL) {
        return -1; // Función inválida
    }
    
    return sys_create_process(name, function, argv);
  }

int64_t my_kill(uint64_t pid) {
    return sys_kill(pid);
  }
  
  int64_t my_block(uint64_t pid) {
    return sys_block(pid);
  }
  
  int64_t my_unblock(uint64_t pid) {
    return sys_unblock(pid);
  }