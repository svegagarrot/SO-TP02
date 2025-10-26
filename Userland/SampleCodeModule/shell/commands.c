#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"
#include "../include/game.h"

extern void _invalidOp();
extern int64_t test_mm(uint64_t argc, char *argv[]);
extern int64_t test_processes(uint64_t argc, char *argv[]);
extern uint64_t test_prio(uint64_t argc, char *argv[]);
extern int mmTypeCmd(int argc, char *argv[]);

const TShellCmd shellCmds[] = {
    {"help", helpCmd, ": Muestra los comandos disponibles\n"},
    {"exit", exitCmd, ": Salir del shell\n"},
    {"set-user", setUserCmd, ": Setea el nombre de usuario, con un maximo de 10 caracteres\n"},
    {"clear", clearCmd, ": Limpia la pantalla\n"},
    {"time", timeCmd, ": Muestra la hora actual\n"},
    {"font-size", fontSizeCmd, ": Cambia el tamanio de la fuente\n"},
    {"testmm", testMMCmd, ": Ejecuta el stress test de memoria. Uso: testmm <max_mem>\n"},
    {"testproceses", testProcesesCmd, ": Ejecuta el stress test de procesos. Uso: testproceses <max_proceses>\n"},
    {"test_priority", testPriorityCmd, ": Ejecuta el test de prioridades. Uso: test_priority <max_value>\n"},
    {"exceptions", exceptionCmd, ": Testear excepciones. Ingrese: exceptions [zero/invalidOpcode] para testear alguna operacion\n"},
    {"jugar", gameCmd, ": Inicia el modo juego\n"},
    {"regs", regsCmd, ": Muestra los ultimos 18 registros de la CPU\n"},
    {"mmtype", mmTypeCmd, ": Muestra el tipo de memory manager activo\n"},
    {NULL, NULL, NULL}, 
};


int regsCmd(int argc, char *argv[]) {
    uint64_t snap[18];
    get_regs(snap); 

    CPURegisters *regs = (CPURegisters *)snap; 

    printf("RAX: %llx\tRBX: %llx\n", regs->rax, regs->rbx);
    printf("RCX: %llx\tRDX: %llx\n", regs->rcx, regs->rdx);
    printf("RSI: %llx\tRDI: %llx\n", regs->rsi, regs->rdi);
    printf("RBP: %llx\tR8 : %llx\n", regs->rbp, regs->r8);
    printf("R9 : %llx\tR10: %llx\n", regs->r9, regs->r10);
    printf("R11: %llx\tR12: %llx\n", regs->r11, regs->r12);
    printf("R13: %llx\tR14: %llx\n", regs->r13, regs->r14);
    printf("R15: %llx\tRIP: %llx\n", regs->r15, regs->rip);
    printf("RSP: %llx\tRFLAGS: %llx\n", regs->rsp, regs->rflags);
    return OK;
}


int helpCmd(int argc, char *argv[]){
    printf("%s", "Comandos disponibles:\n");
    for(int i = 1; shellCmds[i].name; i++){
        printf("%s", shellCmds[i].name);
        printf("%s", shellCmds[i].help);
    }
    return OK;
}

int exitCmd(int argc, char *argv[]) {
    clearScreen();
    printf("%s", "Saliendo del shell...\n");
    sleep(1000);
    shutdown();
    return EXIT_CODE;
}

int setUserCmd(int argc, char *argv[]){
    char newName[MAX_USER_LENGTH + 1];
    
    printf("%s", "Ingrese el nuevo nombre de usuario: ");
    readLine(newName, sizeof(newName));
    
    strncpy(shellUser, newName, MAX_USER_LENGTH);
    printf("Nombre de usuario actualizado a: %s\n", shellUser);
    return OK;
}

int clearCmd(int argc, char *argv[]){
    clearScreen();
    return OK;
}

int timeCmd(int argc, char *argv[]){
    char time[TIME_BUFF];
    getTime(time);
    printf("Hora del sistema: %s\n", time);
    return OK;
}

int fontSizeCmd(int argc, char *argv[]){
    char input[10];
    int size;
    
    printf("Ingrese el nuevo tamanio de la fuente (1-3): ");
    readLine(input, sizeof(input));
    size = atoi(input);
    
    if(size < 1 || size > 3){
        printf("Tamanio invalido. Debe estar entre 1 y 3.\n");
        return CMD_ERROR;
    }
    
    setFontScale(size);
    clearScreen();
    printf("Tamanio de fuente cambiado a: %d\n", size);
    return OK;
}

int CommandParse(char *commandInput){
    if(commandInput == NULL)
        return ERROR;
    
    char *args[MAX_ARGS];
    int argc = fillCommandAndArgs(args, commandInput);

    if(argc == 0)
        return ERROR;

    for(int i = 0; shellCmds[i].name; i++) {
        if(strcmp(args[0], shellCmds[i].name) == 0) {
            return shellCmds[i].function(argc, args);
        }
    }

    return ERROR;
}

int fillCommandAndArgs(char *args[], char *input) {
    int argc = 0;
    int inArg = 0;

    for (int i = 0; input[i] != '\0' && argc < MAX_ARGS; i++) {
        if (input[i] == ' ') {
            input[i] = '\0';
            inArg = 0;
        } else if (!inArg) {
            args[argc++] = &input[i];
            inArg = 1;
        }
    }

    return argc;
}

int exceptionCmd(int argc, char * argv[]) {
    if (argc != 2 || argv[1] == NULL) {
        printf("Error: cantidad invalida de argumentos.\nUso: exceptions [zero, invalidOpcode]\n");
        return CMD_ERROR;
    }

    if (strcmp(argv[1], "zero") == 0) {
        int a = 1;
        int b = 0;
        int c = a / b;   
        printf("c: %d\n", c); 
    }
    else if (strcmp(argv[1], "invalidopcode") == 0) {
        printf("Ejecutando invalidOpcode...\n");
        _invalidOp();   
    }
    else {
        printf("Error: tipo de excepcion invalido.\nIngrese exceptions [zero, invalidOpcode] para testear alguna operacion\n");
        return CMD_ERROR;
    }

    return OK;
}

int testMMCmd(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: testmm <max_mem>\n");
        return CMD_ERROR;
    }

    int64_t result = test_mm(argc - 1, argv + 1);
    if (result == -1) {
        printf("testmm fallo\n");
        return CMD_ERROR;
    }

    return OK;
}

int testProcesesCmd(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: testproceses <max_proceses>\n");
        return CMD_ERROR;
    }

    int64_t result = test_processes(argc - 1, argv + 1);

    if (result == -1) {
        printf("testproceses fallo\n");
        return CMD_ERROR;
    }

    return OK;
}

int gameCmd(int argc, char *argv[]) {
    game_main_screen();
    return OK;
}

int mmTypeCmd(int argc, char *argv[]) {
    char buf[32] = {0};
    if (get_type_of_mm(buf, sizeof(buf))) {
        printf("Memory manager activo: %s\n", buf);
    } else {
        printf("No se pudo obtener el tipo de memory manager\n");
    }
    return OK;
}

int testPriorityCmd(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: test_priority <max_value>\n");
        return CMD_ERROR;
    }
    uint64_t res = test_prio(argc - 1, argv + 1);
    if ((int64_t)res == -1) {
        printf("test_priority fallo\n");
        return CMD_ERROR;
    }
    return OK;
}