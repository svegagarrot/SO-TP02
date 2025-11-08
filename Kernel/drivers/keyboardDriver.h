#ifndef KEYBOARDDRIVER_H
#define KEYBOARDDRIVER_H

#include <stdint.h>
#include <stddef.h>
#include "interrupts.h" 
#include "registers.h"

#define ESC 0x1B
#define BACKSPACE 0x0E
#define ENTER 0x1C
#define SHIFT_L 0x2A
#define SHIFT_R 0x36
#define CTRL_L 0x1D
#define CTRL_L_RELEASE 0x9D
#define CAPSLOCK 0x3A
#define RELEASE_OFFSET 0x80
#define KEY_COUNT 0x54
#define KEYBOARD_DATA_PORT 0x60
#define MAX_SCANCODE 0x58
#define BUFFER_SIZE 256

#define REGISTERS_CANT 18

#define KEY_ARROW_UP     0x80
#define KEY_ARROW_DOWN   0x81
#define KEY_ARROW_LEFT   0x82
#define KEY_ARROW_RIGHT  0x83
#define SC_DELETE 0x53
#define KEY_DELETE 0x84

#define SC_UP     0x48
#define SC_DOWN   0x50
#define SC_LEFT   0x4B
#define SC_RIGHT  0x4D
#define SC_SPACE  0x39
#define SC_TAB    0x0F
#define CTRL_R_CODE 0x12

extern uint8_t getScanCode();
extern void request_snapshot();


/* Manejador de interrupciones para el teclado, 
 * se ejecuta cuando el teclado genera una interrupcion
*/
void keyboard_interrupt_handler();

/* 
 * Devuelve un caracter desde el buffer del teclado o 0 si el buffer esta vacio
*/
char keyboard_read_getchar();

/* Inicializa el subsistema de teclado (semáforos, buffers, etc.) */
void keyboard_init(void);

/* Bloquea hasta que haya al menos un carácter disponible en el buffer */
void keyboard_wait_for_char(void);

/* Limpia el buffer del teclado y resetea el semáforo
 * Se llama cuando se mata un proceso foreground con Ctrl+C */
void keyboard_clear_buffer(void);


#endif 