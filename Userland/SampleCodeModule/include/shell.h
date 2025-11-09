#ifndef SHELL_H
#define SHELL_H

#define MAX_LINE_LENGTH 100
#define MAX_USER_LENGTH 10
#define WELCOME_MESSAGE "Bienvenido a la shell del grupo 28!\n"
#define NOT_FOUND_MESSAGE "Comando no encontrado, escriba help para ver los comandos disponibles\n"
#define FOREGROUND_COLOR 0xFFFFFF
#define BACKGROUND_COLOR 0x000000

extern char shellUser[MAX_USER_LENGTH + 1];

void shellLoop();

void readLine(char *buf, int maxLen);

#endif
