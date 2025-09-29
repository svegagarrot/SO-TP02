#ifndef VIDEO_DRIVER_H
#define VIDEO_DRIVER_H

#include <stdint.h>
#include <stddef.h>

#define BACKGROUND_COLOR 0x000000
#define FOREGROUND_COLOR 0xFFFFFF
#define ERROR_FG_COLOR 0xFF0000
#define ERROR_BG_COLOR 0x000000
#define REG_NAME_COLOR 0xFFFFFF
#define REG_VALUE_COLOR 0x00FF00
#define REG_BG_COLOR 0x000000


typedef struct vbe_mode_info_structure *VBEinfoPtr;
 
void video_putPixel(uint32_t color, uint64_t x, uint64_t y);
void video_putChar(char c, uint64_t foregroundColor, uint64_t backgroundColor);
void video_clearScreen();
void video_putString(char *string, uint64_t foregroundColor, uint64_t backgroundColor);
void video_newLine();
void video_backSpace();
void video_tab();
void video_moveCursorLeft();
void video_moveCursorRight();
void video_moveCursorUp();
void video_moveCursorDown();
void video_drawCursor(uint64_t color);
void setFontScale(uint64_t scale);
void video_printError(const char *errorMsg);
void video_clearScreenColor(uint32_t color);
void video_putCharXY(char c, int x, int y, uint32_t fg, uint32_t bg);

uint16_t video_get_width();
uint16_t video_get_height();

#endif