#include <font.h>
#include <videoDriver.h>

static int isSpecialChar(char c);
static void video_scrollUp();

struct vbe_mode_info_structure {
    uint16_t attributes;      
    uint8_t window_a;   
    uint8_t window_b;         
    uint16_t granularity;      
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;   
    uint16_t pitch;           
    uint16_t width;          
    uint16_t height;        
    uint8_t w_char;            
    uint8_t y_char;           
    uint8_t planes;
    uint8_t bpp;       
    uint8_t banks;           
    uint8_t memory_model;
    uint8_t bank_size;        
    uint8_t image_pages;
    uint8_t reserved0;
    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;
    uint32_t framebuffer;      
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size;
    uint8_t reserved1[206];
} __attribute__ ((packed));

VBEinfoPtr VBEModeInfo = (VBEinfoPtr) 0x0000000000005C00;
#define SCREEN_WIDTH  (VBEModeInfo->width)
#define SCREEN_HEIGHT (VBEModeInfo->height)


uint64_t cursorX = 0;
uint64_t cursorY = 0;
uint64_t fontScale = 1;

void video_putPixel(uint32_t color, uint64_t x, uint64_t y) {
    uint8_t *framebuffer = (uint8_t *)(uintptr_t) VBEModeInfo->framebuffer;
    uint64_t offset = (x * ((VBEModeInfo->bpp)/8)) + (y * VBEModeInfo->pitch);
    framebuffer[offset]     = (color) & 0xFF;
    framebuffer[offset+1]   = (color >> 8) & 0xFF;
    framebuffer[offset+2]   = (color >> 16) & 0xFF;
}

void video_putChar(char c, uint64_t foregroundColor, uint64_t backgroundColor) {

    if (isSpecialChar(c)) {
        return;
    }

    uint8_t charIndex = (uint8_t)c;
    unsigned char* charPattern = font8x16[charIndex];

    for (int y = 0; y < FONT_HEIGHT; y++) {
        for (int x = 0; x < FONT_WIDTH; x++) {
            uint32_t color = (charPattern[y] & (1 << (7 - x))) ? foregroundColor : backgroundColor;
            for (int sy = 0; sy < fontScale; sy++) {
                for (int sx = 0; sx < fontScale; sx++) {
                    video_putPixel(color, cursorX + (x * fontScale) + sx, cursorY + (y * fontScale) + sy);
                }
            }
        }
    }

    cursorX += FONT_WIDTH * fontScale;
    if (cursorX >= VBEModeInfo->width) {
        video_newLine();
    }
}

static int isSpecialChar(char c) {
    switch (c) {
        case '\n':
            video_newLine();
            return 1;
        case '\b':
            video_backSpace();
            return 1;
        case '\t':
            video_tab();
            return 1;
    }
    return 0;
}

void video_clearScreen() {
    for (int i = 0; i < VBEModeInfo->height; i++) {
        for (int j = 0; j < VBEModeInfo->width; j++) {
            video_putPixel(BACKGROUND_COLOR, j, i);
        }
    }
    cursorX = 0;
    cursorY = 0;
}

void video_putString(char *string, uint64_t foregroundColor, uint64_t backgroundColor) {
    while (*string) {
        video_putChar(*string, foregroundColor, backgroundColor);
        string++;
    }
}

void video_newLine() {
    cursorX = 0;
    if (cursorY + (FONT_HEIGHT * fontScale) < VBEModeInfo->height) {
        cursorY += FONT_HEIGHT * fontScale;
    } else {
        video_scrollUp();
        cursorY = VBEModeInfo->height - (FONT_HEIGHT * fontScale);
    }
}

void video_backSpace() {
    if (cursorX >= FONT_WIDTH * fontScale) {
        cursorX -= FONT_WIDTH * fontScale;
        for (int i = 0; i < FONT_WIDTH * fontScale; i++) {
            for (int j = 0; j < FONT_HEIGHT * fontScale; j++) {
                video_putPixel(BACKGROUND_COLOR, cursorX + i, cursorY + j);
            }
        }
    } else if (cursorY >= FONT_HEIGHT * fontScale) {
        cursorY -= FONT_HEIGHT * fontScale;
        cursorX = VBEModeInfo->width - (FONT_WIDTH * fontScale);
        for (int i = 0; i < FONT_WIDTH * fontScale; i++) {
            for (int j = 0; j < FONT_HEIGHT * fontScale; j++) {
                video_putPixel(BACKGROUND_COLOR, cursorX + i, cursorY + j);
            }
        }
    }
}

void video_tab() {
    uint64_t step = FONT_WIDTH * fontScale * 8; 
    cursorX = ((cursorX / step) + 1) * step;
    if (cursorX >= VBEModeInfo->width) {
        video_newLine();
    }
}

void video_moveCursorLeft() {
    if (cursorX >= FONT_WIDTH * fontScale) {
        cursorX -= FONT_WIDTH * fontScale;
    } else if (cursorY >= FONT_HEIGHT * fontScale) {
        cursorY -= FONT_HEIGHT * fontScale;
        cursorX = VBEModeInfo->width - (FONT_WIDTH * fontScale);
    }
}

void video_moveCursorRight() {
    if (cursorX + (FONT_WIDTH * fontScale) < VBEModeInfo->width) {
        cursorX += FONT_WIDTH * fontScale;
    } else if (cursorY + (FONT_HEIGHT * fontScale) < VBEModeInfo->height) {
        cursorX = 0;
        cursorY += FONT_HEIGHT * fontScale;
    }
}

void video_moveCursorUp() {
    if (cursorY >= FONT_HEIGHT * fontScale) {
        cursorY -= FONT_HEIGHT * fontScale;
    }
}

void video_moveCursorDown() {
    if (cursorY + (FONT_HEIGHT * fontScale) < VBEModeInfo->height) {
        cursorY += FONT_HEIGHT * fontScale;
    }
}

void video_drawCursor(uint64_t color) {
    for (int x = 0; x < FONT_WIDTH * fontScale; x++) {
        video_putPixel(color, cursorX + x, cursorY + (FONT_HEIGHT * fontScale) - 2);
    }
}

static void video_scrollUp() {
    uint8_t *fb = (uint8_t *)(unsigned long)VBEModeInfo->framebuffer;
    int row_bytes = VBEModeInfo->pitch;
    int n_rows = VBEModeInfo->height;
    int move_bytes = (n_rows - (FONT_HEIGHT * fontScale)) * row_bytes;
    uint8_t *src = fb + (FONT_HEIGHT * fontScale) * row_bytes;
    uint8_t *dst = fb;

    for (int i = 0; i < move_bytes; i++) {
        dst[i] = src[i];
    }

    for (int y = n_rows - (FONT_HEIGHT * fontScale); y < n_rows; y++) {
        for (int x = 0; x < VBEModeInfo->width; x++) {
            video_putPixel(BACKGROUND_COLOR, x, y);
        }
    }
}

void video_printError(const char *errorMsg) {
    video_putString((char *)errorMsg, ERROR_FG_COLOR, ERROR_BG_COLOR);

}

void video_clearScreenColor(uint32_t color) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            video_putPixel(color, x, y);
        }
    }
    cursorX = 0;
    cursorY = 0;
}

void video_putCharXY(char c, int x, int y, uint32_t fg, uint32_t bg) {
    if (c < 0x20 || c > 0x7F) c = 0x20;
    unsigned char *glyph = font8x16[(unsigned char)c];
    for (int row = 0; row < FONT_HEIGHT; row++) {
        for (int col = 0; col < FONT_WIDTH; col++) {
            uint32_t color = (glyph[row] & (1 << (7 - col))) ? fg : bg;
            video_putPixel(color, x + col, y + row);
        }
    }
}

uint16_t video_get_width() {
    return VBEModeInfo->width;
}

uint16_t video_get_height() {
    return VBEModeInfo->height;
}

