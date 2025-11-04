#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"
#include "../include/game.h"

extern void _invalidOp();
extern int64_t test_mm(uint64_t argc, char *argv[]);
extern int64_t test_processes(uint64_t argc, char *argv[]);
extern uint64_t test_prio(uint64_t argc, char *argv[]);
extern uint64_t test_sync(uint64_t argc, char *argv[]);
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
    {"test_sync", testSyncCmd, ": Ejecuta test sincronizado con semaforos. Uso: test_sync <repeticiones>\n"},
    {"test_no_synchro", testNoSynchroCmd, ": Ejecuta test sin sincronizacion. Uso: test_no_synchro <repeticiones>\n"},
    {"test_priority", testPriorityCmd, ": Ejecuta el test de prioridades. Uso: test_priority <max_value>\n"},
    {"exceptions", exceptionCmd, ": Testear excepciones. Ingrese: exceptions [zero/invalidOpcode] para testear alguna operacion\n"},
    {"jugar", gameCmd, ": Inicia el modo juego\n"},
    {"regs", regsCmd, ": Muestra los ultimos 18 registros de la CPU\n"},
    {"mmtype", mmTypeCmd, ": Muestra el tipo de memory manager activo\n"},
    {"ps", psCmd, ": Lista todos los procesos con sus propiedades\n"},
    {"loop", loopCmd, ": Crea un proceso que imprime su ID con saludo cada X segundos. Uso: loop <segundos>\n"},
    {"kill", killCmd, ": Mata un proceso por PID. Uso: kill <pid>\n"},
    {"nice", niceCmd, ": Cambia la prioridad de un proceso. Uso: nice <pid> <prioridad>\n"},
    {"block", blockCmd, ": Cambia el estado de un proceso entre bloqueado y listo. Uso: block <pid>\n"},
    {"mem", memCmd, ": Imprime el estado de la memoria\n"},
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

int testSyncCmd(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Uso: test_sync <repeticiones> [num_pares]\n");
        printf("  repeticiones: número de iteraciones por proceso\n");
        printf("  num_pares: número de pares de procesos (opcional, default=2)\n");
        return CMD_ERROR;
    }

    /* build argv for test_sync: { repetitions, use_sem, [num_pairs] }
       use_sem = 1 for synchronized test */
    char *targv[3];
    targv[0] = argv[1];
    targv[1] = "1";
    if (argc == 3) {
        targv[2] = argv[2];  // número de pares especificado
    }

    int64_t res = (int64_t)test_sync(argc == 3 ? 3 : 2, targv);
    if (res == -1) {
        printf("test_sync fallo\n");
        return CMD_ERROR;
    }
    return OK;
}

int testNoSynchroCmd(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Uso: test_no_synchro <repeticiones> [num_pares]\n");
        printf("  repeticiones: número de iteraciones por proceso\n");
        printf("  num_pares: número de pares de procesos (opcional, default=2)\n");
        return CMD_ERROR;
    }

    char *targv[3];
    targv[0] = argv[1];
    targv[1] = "0"; /* do not use sem */
    if (argc == 3) {
        targv[2] = argv[2];  // número de pares especificado
    }

    int64_t res = (int64_t)test_sync(argc == 3 ? 3 : 2, targv);
    if (res == -1) {
        printf("test_no_synchro fallo\n");
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

static const char *state_to_string(int state) {
    switch (state) {
        case PROCESS_STATE_NEW: return "NEW";
        case PROCESS_STATE_READY: return "READY";
        case PROCESS_STATE_RUNNING: return "RUNNING";
        case PROCESS_STATE_BLOCKED: return "BLOCKED";
        case PROCESS_STATE_FINISHED: return "FINISHED";
        default: return "UNKNOWN";
    }
}

int psCmd(int argc, char *argv[]) {
    (void)argc;  // No se usan argumentos
    (void)argv;

    process_info_t processes[MAX_PROCESS_INFO];
    uint64_t count = list_processes(processes, MAX_PROCESS_INFO);

    if (count == 0) {
        printf("No hay procesos en el sistema.\n");
        return OK;
    }

    printf("\nPID\tNombre\t\t\tEstado\t\tPrioridad\tRSP\t\t\tRBP\t\t\tForeground\n");
    printf("---------------------------------------------------------------------------------------------------------------------------\n");

    for (uint64_t i = 0; i < count; i++) {
        printf("%llu\t", processes[i].pid);
        printf("%s\t\t", processes[i].name);
        if (strlen(processes[i].name) < 8) {
            printf("\t");
        }
        printf("%s\t\t", state_to_string(processes[i].state));
        printf("%d\t\t", processes[i].priority);
        printf("0x%llx\t", processes[i].rsp);
        printf("0x%llx\t", processes[i].rbp);
        printf("%s\n", processes[i].foreground ? "Si" : "No");
    }

    printf("\nTotal de procesos: %llu\n", count);
    return OK;
}

static void loop_process_entry(void *arg) {
    int seconds = 1;  // Default
    char **argv = (char **)arg;
    
    if (argv != NULL && argv[0] != NULL) {
        seconds = atoi(argv[0]);
        if (seconds <= 0) {
            seconds = 1;  // Fallback a default
        }
        // Liberar la memoria del string copiado
        free(argv[0]);
        free(argv);
    }
    
    int64_t pid = my_getpid();
    
    while (1) {
        printf("Hola! Soy el proceso con ID %lld\n", pid);
        sleep(seconds * 1000);  // sleep espera milisegundos
    }
}

int loopCmd(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: loop <segundos>\n");
        return CMD_ERROR;
    }

    int seconds = atoi(argv[1]);
    if (seconds <= 0) {
        printf("Error: el tiempo debe ser un numero positivo.\n");
        return CMD_ERROR;
    }

    // Crear una copia permanente del string del argumento
    char *seconds_str = (char *)malloc(32);  // Suficiente para cualquier número
    if (seconds_str == NULL) {
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    
    // Copiar el string del argumento
    int i = 0;
    while (argv[1][i] != '\0' && i < 31) {
        seconds_str[i] = argv[1][i];
        i++;
    }
    seconds_str[i] = '\0';
    
    // Crear un array con el argumento para pasarlo al proceso
    char **process_argv = (char **)malloc(2 * sizeof(char *));
    if (process_argv == NULL) {
        free(seconds_str);
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    
    process_argv[0] = seconds_str;  // El número de segundos como string (copiado)
    process_argv[1] = NULL;

    int64_t pid = my_create_process("loop_process", loop_process_entry, process_argv);
    if (pid <= 0) {
        free(seconds_str);
        free(process_argv);
        printf("Error: no se pudo crear el proceso loop.\n");
        return CMD_ERROR;
    }

    printf("Proceso loop creado con PID: %lld\n", pid);
    return OK;
}

int killCmd(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: kill <pid>\n");
        return CMD_ERROR;
    }

    int64_t pid = atoi(argv[1]);
    if (pid <= 0) {
        printf("Error: PID invalido.\n");
        return CMD_ERROR;
    }

    int64_t result = my_kill(pid);
    if (result == 0) {
        printf("Error: no se pudo matar el proceso %lld. Puede que no exista.\n", pid);
        return CMD_ERROR;
    }

    printf("Proceso %lld terminado exitosamente.\n", pid);
    return OK;
}

int niceCmd(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: nice <pid> <prioridad>\n");
        return CMD_ERROR;
    }

    int64_t pid = atoi(argv[1]);
    if (pid <= 0) {
        printf("Error: PID invalido.\n");
        return CMD_ERROR;
    }

    int priority = atoi(argv[2]);
    if (priority < 0 || priority > 2) {
        printf("Error: la prioridad debe estar entre 0 y 2.\n");
        return CMD_ERROR;
    }

    int64_t result = my_nice(pid, priority);
    if (result == 0) {
        printf("Error: no se pudo cambiar la prioridad del proceso %lld.\n", pid);
        return CMD_ERROR;
    }

    printf("Prioridad del proceso %lld cambiada a %d exitosamente.\n", pid, priority);
    return OK;
}

int blockCmd(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: block <pid>\n");
        return CMD_ERROR;
    }

    int64_t pid = atoi(argv[1]);
    if (pid <= 0) {
        printf("Error: PID invalido.\n");
        return CMD_ERROR;
    }

    // Para determinar si está bloqueado, consultamos la lista de procesos
    process_info_t processes[MAX_PROCESS_INFO];
    uint64_t count = list_processes(processes, MAX_PROCESS_INFO);
    
    int found = 0;
    int is_blocked = 0;
    
    for (uint64_t i = 0; i < count; i++) {
        if (processes[i].pid == (uint64_t)pid) {
            found = 1;
            is_blocked = (processes[i].state == PROCESS_STATE_BLOCKED);
            break;
        }
    }

    if (!found) {
        printf("Error: proceso %lld no encontrado.\n", pid);
        return CMD_ERROR;
    }

    int64_t result;
    if (is_blocked) {
        result = my_unblock(pid);
        if (result == 0) {
            printf("Error: no se pudo desbloquear el proceso %lld.\n", pid);
            return CMD_ERROR;
        }
        printf("Proceso %lld desbloqueado exitosamente.\n", pid);
    } else {
        result = my_block(pid);
        if (result == 0) {
            printf("Error: no se pudo bloquear el proceso %lld.\n", pid);
            return CMD_ERROR;
        }
        printf("Proceso %lld bloqueado exitosamente.\n", pid);
    }

    return OK;
}

int memCmd(int argc, char *argv[]) {
    (void)argc;  // No se usan argumentos
    (void)argv;

    memory_info_t info;
    if (memory_info(&info) == 0) {
        printf("Error: no se pudo obtener la informacion de memoria.\n");
        return CMD_ERROR;
    }

    printf("\n=== Estado de la Memoria ===\n");
    printf("Total de bytes:       %llu\n", info.total_bytes);
    printf("Bytes usados:         %llu\n", info.used_bytes);
    printf("Bytes libres:         %llu\n", info.free_bytes);
    printf("Bloque libre mas grande: %llu bytes\n", info.largest_free_block);
    printf("Asignaciones totales: %llu\n", info.allocations);
    printf("Liberaciones totales: %llu\n", info.frees);
    printf("Asignaciones fallidas: %llu\n", info.failed_allocations);
    
    // Calcular porcentaje de uso
    if (info.total_bytes > 0) {
        uint64_t used_percent = (info.used_bytes * 100) / info.total_bytes;
        uint64_t free_percent = (info.free_bytes * 100) / info.total_bytes;
        printf("\nPorcentaje de uso:   %llu%%\n", used_percent);
        printf("Porcentaje libre:    %llu%%\n", free_percent);
    }
    
    printf("\n");
    return OK;
}