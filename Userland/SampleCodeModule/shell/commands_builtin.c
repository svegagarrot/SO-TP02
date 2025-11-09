#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"

extern void _invalidOp();

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

int exceptionCmd(int argc, char * argv[]) {
    if (argc != 2 || argv[1] == NULL) {
        printf("Error: cantidad invalida de argumentos.\nUso: exceptions [zero, invalidOpcode]\n");
        return CMD_ERROR;
    }

    if (strcmp(argv[1], "zero") == 0) {
        int a = 1;
        int b = 0;
        // División por cero intencional para probar manejo de excepciones
        //-V609
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
