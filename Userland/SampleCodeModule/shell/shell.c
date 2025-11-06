#include "../include/shell.h"
#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/syscall.h"
static void shellPrompt();

char shellUser[MAX_USER_LENGTH + 1] = "usuario";
void shellLoop() {
    char buffer[MAX_LINE_LENGTH];

    printf("%s", WELCOME_MESSAGE);

    while (1) {
        shellPrompt();
        readLine(buffer, MAX_LINE_LENGTH);

        size_t len = strlen(buffer);
        if (len == 0) continue;

        if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';

        for(int i=0; buffer[i]!='\0'; i++){
            if(buffer[i] >= 'A' && buffer[i] <= 'Z'){
                buffer[i] = buffer[i] + 32;
            }
        }

        int result = CommandParse(buffer);
        if (result == ERROR) {
            printf("%s", NOT_FOUND_MESSAGE);
        } else if (result == EXIT_CODE) {
            break; 
        }
    }
}

void readLine(char *buf, int maxLen) {
    int len = 0;
    int c;

    while (1) {
        c = getchar();
        
        // Ctrl+D (0x04): enviar EOF
        if (c == 0x04) {
            // Si la línea está vacía, comportamiento típico de EOF
            if (len == 0) {
                printf("\n");
                buf[0] = '\0';
                return;
            }
            // Si hay contenido, ignorar Ctrl+D
            continue;
        }
        
        
        if (c == '\n') {
            putchar('\n');
            break;
        }
        if (c == '\b') {
            if (len > 0) {
                len--;
                putchar('\b'); putchar(' '); putchar('\b');
            }
            else{
                sys_beep(500, 100);
            }
        } else if (len < maxLen-1) {
            buf[len++] = (char)c;
            putchar(c);
        }
    }
    buf[len] = '\0';
}

static void shellPrompt(){
    printf("%s", "TP-ARQUI-");
    printf("%s", shellUser);
    printf("%s", ":~$ ");
}