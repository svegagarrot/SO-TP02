#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"

extern int g_run_in_background;

int psCmd(int argc, char *argv[]) {
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("ps", ps_process_entry, NULL, is_foreground, 0, 0);
    
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso ps.\n");
        return CMD_ERROR;
    }
    
    if (g_run_in_background) {
        printf("Proceso ps creado con PID: %lld (background)\n", pid);
    }
    
    return OK;
}

int loopCmd(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: loop <segundos> [&]\n");
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
    // Verificar límites antes de acceder al array
    while (i < 31 && argv[1][i] != '\0') {
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

    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("loop_process", loop_process_entry, process_argv, is_foreground, 0, 0);
    
    if (pid <= 0) {
        free(seconds_str);
        free(process_argv);
        printf("Error: no se pudo crear el proceso loop.\n");
        return CMD_ERROR;
    }

    printf("Proceso loop creado con PID: %lld%s\n", pid, g_run_in_background ? " (background)" : "");
    
    // Nota: execute_external_command ya esperó si es foreground
    if (!g_run_in_background) {
        printf("\nProceso loop terminado.\n");
    }
    
    return OK;
}

int memCmd(int argc, char *argv[]) {
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("mem", mem_process_entry, NULL, is_foreground, 0, 0);
    
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso mem.\n");
        return CMD_ERROR;
    }
    
    if (g_run_in_background) {
        printf("Proceso mem creado con PID: %lld (background)\n", pid);
    }
    
    return OK;
}

int mmTypeCmd(int argc, char *argv[]) {
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("mmtype", mmtype_process_entry, NULL, is_foreground, 0, 0);
    
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso mmtype.\n");
        return CMD_ERROR;
    }
    
    if (g_run_in_background) {
        printf("Proceso mmtype creado con PID: %lld (background)\n", pid);
    }
    
    return OK;
}
