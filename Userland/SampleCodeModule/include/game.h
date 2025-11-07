#ifndef GAME_H
#define GAME_H

#include <stdint.h>

// Definiciones de scancodes de teclado
#define SC_UP     0x48
#define SC_DOWN   0x50
#define SC_LEFT   0x4B
#define SC_RIGHT  0x4D
#define SC_SPACE  0x39
#define SC_ESC    0x01
#define SC_ENTER  0x1C

// Scancodes para teclas alfabéticas
#define SC_W      0x11
#define SC_A      0x1E
#define SC_D      0x20

// Scancodes para teclas numéricas
#define SC_1      0x02
#define SC_2      0x03

void game_main_screen(void);
void game_start(int num_players);
#endif