#include "keyboardDriver.h"
#include "keystate.h"
#include <naiveConsole.h>
#include "semaphore.h"
#include "scheduler.h"

static int buffer_empty();
static int buffer_full();
static char buffer_pop();
static char buffer_push(char c);
static char scToAscii(uint8_t scancode);
static void updateFlags(uint8_t scancode);

//Flags teclas especiales
static volatile uint8_t activeShift = 0;               //Shift presionado
static volatile uint8_t activeCapsLock = 0;            //CapsLock presionado
static volatile uint8_t activeCtrl = 0;                //Ctrl presionado

// Arreglo para mantener el estado de las teclas
uint8_t key_states[256] = {0};

typedef struct CircleBuffer{
    char buffer[BUFFER_SIZE];
    int readIndex;          
    int writeIndex;          
    int size;
} TCircleBuffer;


static TCircleBuffer buffer = {.readIndex = 0, .writeIndex = 0, .size = 0};
static uint64_t kbd_sem_id = 0;

// En primer indice char sin shift, en segundo indice char con shift
static const char scancode_table[KEY_COUNT][2] = {
    {0, 0}, {ESC, ESC}, {'1', '!'}, {'2', '@'}, {'3', '#'},
    {'4', '$'}, {'5', '%'}, {'6', '^'}, {'7', '&'}, {'8', '*'},
    {'9', '('}, {'0', ')'}, {'-', '_'}, {'=', '+'}, {'\b', '\b'},
    {'\t', '\t'}, {'q', 'Q'}, {'w', 'W'}, {'e', 'E'}, {'r', 'R'},
    {'t', 'T'}, {'y', 'Y'}, {'u', 'U'}, {'i', 'I'}, {'o', 'O'},
    {'p', 'P'}, {'[', '{'}, {']', '}'}, {'\n', '\n'}, {0, 0},
    {'a', 'A'}, {'s', 'S'}, {'d', 'D'}, {'f', 'F'}, {'g', 'G'},
    {'h', 'H'}, {'j', 'J'}, {'k', 'K'}, {'l', 'L'}, {';', ':'},
    {'\'', '\"'}, {'`', '~'}, {0, 0}, {'\\', '|'}, {'z', 'Z'},
    {'x', 'X'}, {'c', 'C'}, {'v', 'V'}, {'b', 'B'}, {'n', 'N'},
    {'m', 'M'}, {',', '<'}, {'.', '>'}, {'/', '?'}, {0, 0}, {0, 0},
    {0, 0}, {' ', ' '},
};

void keyboard_interrupt_handler() {
    uint8_t scancode = getScanCode();           
    updateFlags(scancode);                     

    if (scancode & RELEASE_OFFSET) {
        key_states[scancode & ~RELEASE_OFFSET] = 0;
    } else {
        key_states[scancode] = 1;
    }

    char cAscii = scToAscii(scancode);        

    if (activeCtrl && (cAscii == 'r' || cAscii == 'R')) {
        request_snapshot();
    } else if (activeCtrl && (cAscii == 'c' || cAscii == 'C')) {
        // Ctrl+C: matar proceso en foreground inmediatamente
        uint64_t fg_pid = scheduler_get_foreground_pid();
        if (fg_pid != 0) {
            // Imprimir ^C en consola
            ncPrint("^C\n");
            scheduler_kill_by_pid(fg_pid);
        }
    } else if (activeCtrl && (cAscii == 'd' || cAscii == 'D')) {
        // Ctrl+D: enviar carácter especial 0x04 para EOF
        if (buffer_push(0x04)) {
            if (kbd_sem_id != 0) {
                sem_signal_by_id(kbd_sem_id);
            }
        }
    } else if (cAscii != 0) {
        if (buffer_push(cAscii)) {
            if (kbd_sem_id != 0) {
                sem_signal_by_id(kbd_sem_id);
            }
        }
    }
}

static void updateFlags(uint8_t scancode) {
    if (scancode == CTRL_L) {
        activeCtrl = 1;
    }
    else if (scancode == CTRL_L_RELEASE) {
        activeCtrl = 0;
    }
    else if (scancode == SHIFT_L || scancode == SHIFT_R) {
        activeShift = 1;
    }
    else if (scancode == (SHIFT_L + RELEASE_OFFSET) || scancode == (SHIFT_R + RELEASE_OFFSET)) {
        activeShift = 0;
    }
    else if (scancode == CAPSLOCK) {
        activeCapsLock = !activeCapsLock;
    }
}


static char buffer_push(char c) {
    if (buffer_full()) return 0;

    buffer.buffer[buffer.writeIndex] = c;
    buffer.writeIndex = (buffer.writeIndex + 1) % BUFFER_SIZE;
    buffer.size++;
    return 1;
}

static char buffer_pop() {
    if (buffer_empty()){
        return 0;
    }

    char c = buffer.buffer[buffer.readIndex];
    buffer.readIndex = (buffer.readIndex + 1) % BUFFER_SIZE;
    buffer.size--;
    return c;
}

static int buffer_full() {
    return buffer.size == BUFFER_SIZE;
}

static int buffer_empty() {
    return buffer.size == 0;
}

char keyboard_read_getchar() {
    return buffer_pop();
}

void keyboard_init(void) {
    if (kbd_sem_id == 0) {
        kbd_sem_id = sem_alloc(0);
    }
}

void keyboard_wait_for_char(void) {
    while (buffer_empty()) {
        if (kbd_sem_id != 0) {
            sem_wait_by_id(kbd_sem_id);
        }
    }
}

static char scToAscii(uint8_t scancode) {
    switch (scancode) {
        case 1:
        case SC_UP:
        case SC_DOWN:
        case SC_LEFT:
        case SC_RIGHT:
        case ESC:
        case SC_DELETE:
            return 0; 
        default:
            break;
    }

    if (scancode < KEY_COUNT) {
        char c = scancode_table[scancode][activeShift];
        if (activeCapsLock && c >= 'a' && c <= 'z') {
            c -= 32;
        }
        return c;
    }
    return 0;
}

uint8_t is_key_pressed(uint8_t scancode) {
    if (scancode < 256) {
        return key_states[scancode];
    }
    return 0;
}