#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"

extern int g_run_in_background;

int testMMCmd(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: testmm <max_mem> [&]\n");
        return CMD_ERROR;
    }

    // Copiar argumentos para el proceso
    char **process_argv = (char **)malloc(2 * sizeof(char *));
    if (process_argv == NULL) {
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    
    char *arg_copy = (char *)malloc(strlen(argv[1]) + 1);
    if (arg_copy == NULL) {
        free(process_argv);
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    
    // Copiar manualmente
    int idx = 0;
    while (argv[1][idx] != '\0') {
        arg_copy[idx] = argv[1][idx];
        idx++;
    }
    arg_copy[idx] = '\0';
    process_argv[0] = arg_copy;
    process_argv[1] = NULL;
    
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("test_mm", test_mm_process_wrapper, process_argv, is_foreground, 0, 0);
    
    if (pid <= 0) {
        free(arg_copy);
        free(process_argv);
        printf("Error: no se pudo crear el proceso test_mm.\n");
        return CMD_ERROR;
    }
    
    if (g_run_in_background) {
        printf("Test de memoria iniciado con PID: %lld (background)\n", pid);
    }
    
    return OK;
}

int testSyncCmd(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Uso: test_sync <repeticiones> [num_pares] [&]\n");
        printf("  repeticiones: número de iteraciones por proceso\n");
        printf("  num_pares: número de pares de procesos (opcional, default=2)\n");
        return CMD_ERROR;
    }

    // Necesitamos 2 o 3 argumentos: repeticiones, [num_pares]
    int num_args = (argc == 3) ? 2 : 1;
    char **process_argv = (char **)malloc((num_args + 1) * sizeof(char *));
    if (process_argv == NULL) {
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    
    // Copiar repeticiones
    char *arg1_copy = (char *)malloc(strlen(argv[1]) + 1);
    if (arg1_copy == NULL) {
        free(process_argv);
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    int idx = 0;
    while (argv[1][idx] != '\0') {
        arg1_copy[idx] = argv[1][idx];
        idx++;
    }
    arg1_copy[idx] = '\0';
    process_argv[0] = arg1_copy;
    
    // Copiar num_pares si existe
    if (argc == 3) {
        char *arg2_copy = (char *)malloc(strlen(argv[2]) + 1);
        if (arg2_copy == NULL) {
            free(arg1_copy);
            free(process_argv);
            printf("Error: no se pudo asignar memoria para el proceso.\n");
            return CMD_ERROR;
        }
        idx = 0;
        while (argv[2][idx] != '\0') {
            arg2_copy[idx] = argv[2][idx];
            idx++;
        }
        arg2_copy[idx] = '\0';
        process_argv[1] = arg2_copy;
        process_argv[2] = NULL;
    } else {
        process_argv[1] = NULL;
    }
    
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("test_synchro", test_sync_wrapper, process_argv, is_foreground, 0, 0);
    
    if (pid <= 0) {
        free(arg1_copy);
        if (argc == 3) free(process_argv[1]);
        free(process_argv);
        printf("Error: no se pudo crear el proceso test_synchro.\n");
        return CMD_ERROR;
    }
    
    if (g_run_in_background) {
        printf("Test de sincronizacion iniciado con PID: %lld (background)\n", pid);
    }
    
    return OK;
}

int testNoSynchroCmd(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Uso: test_no_synchro <repeticiones> [num_pares] [&]\n");
        printf("  repeticiones: número de iteraciones por proceso\n");
        printf("  num_pares: número de pares de procesos (opcional, default=2)\n");
        return CMD_ERROR;
    }

    // Necesitamos 2 o 3 argumentos: repeticiones, [num_pares]
    int num_args = (argc == 3) ? 2 : 1;
    char **process_argv = (char **)malloc((num_args + 1) * sizeof(char *));
    if (process_argv == NULL) {
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    
    // Copiar repeticiones
    char *arg1_copy = (char *)malloc(strlen(argv[1]) + 1);
    if (arg1_copy == NULL) {
        free(process_argv);
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    int idx = 0;
    while (argv[1][idx] != '\0') {
        arg1_copy[idx] = argv[1][idx];
        idx++;
    }
    arg1_copy[idx] = '\0';
    process_argv[0] = arg1_copy;
    
    // Copiar num_pares si existe
    if (argc == 3) {
        char *arg2_copy = (char *)malloc(strlen(argv[2]) + 1);
        if (arg2_copy == NULL) {
            free(arg1_copy);
            free(process_argv);
            printf("Error: no se pudo asignar memoria para el proceso.\n");
            return CMD_ERROR;
        }
        idx = 0;
        while (argv[2][idx] != '\0') {
            arg2_copy[idx] = argv[2][idx];
            idx++;
        }
        arg2_copy[idx] = '\0';
        process_argv[1] = arg2_copy;
        process_argv[2] = NULL;
    } else {
        process_argv[1] = NULL;
    }
    
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("test_no_synchro", test_no_synchro_wrapper, process_argv, is_foreground, 0, 0);
    
    if (pid <= 0) {
        free(arg1_copy);
        if (argc == 3) free(process_argv[1]);
        free(process_argv);
        printf("Error: no se pudo crear el proceso test_no_synchro.\n");
        return CMD_ERROR;
    }
    
    if (g_run_in_background) {
        printf("Test sin sincronizacion iniciado con PID: %lld (background)\n", pid);
    }
    
    return OK;
}

int testProcesesCmd(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: test_proceses <max_proceses> [&]\n");
        return CMD_ERROR;
    }

    // Copiar argumentos para el proceso
    char **process_argv = (char **)malloc(2 * sizeof(char *));
    if (process_argv == NULL) {
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    
    char *arg_copy = (char *)malloc(strlen(argv[1]) + 1);
    if (arg_copy == NULL) {
        free(process_argv);
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    
    // Copiar manualmente
    int idx = 0;
    while (argv[1][idx] != '\0') {
        arg_copy[idx] = argv[1][idx];
        idx++;
    }
    arg_copy[idx] = '\0';
    process_argv[0] = arg_copy;
    process_argv[1] = NULL;
    
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("test_processes", test_processes_wrapper, process_argv, is_foreground, 0, 0);
    
    if (pid <= 0) {
        free(arg_copy);
        free(process_argv);
        printf("Error: no se pudo crear el proceso test_processes.\n");
        return CMD_ERROR;
    }
    
    if (g_run_in_background) {
        printf("Test de procesos iniciado con PID: %lld (background)\n", pid);
    }

    return OK;
}

int testPriorityCmd(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Uso: test_priority <max_value> [&]\n");
        return CMD_ERROR;
    }
    
    // Copiar argumentos para el proceso
    char **process_argv = (char **)malloc(2 * sizeof(char *));
    if (process_argv == NULL) {
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    
    char *arg_copy = (char *)malloc(strlen(argv[1]) + 1);
    if (arg_copy == NULL) {
        free(process_argv);
        printf("Error: no se pudo asignar memoria para el proceso.\n");
        return CMD_ERROR;
    }
    
    // Copiar manualmente
    int idx = 0;
    while (argv[1][idx] != '\0') {
        arg_copy[idx] = argv[1][idx];
        idx++;
    }
    arg_copy[idx] = '\0';
    process_argv[0] = arg_copy;
    process_argv[1] = NULL;
    
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("test_prio", test_prio_wrapper, process_argv, is_foreground, 0, 0);
    
    if (pid <= 0) {
        free(arg_copy);
        free(process_argv);
        printf("Error: no se pudo crear el proceso test_prio.\n");
        return CMD_ERROR;
    }
    
    if (g_run_in_background) {
        printf("Test de prioridades iniciado con PID: %lld (background)\n", pid);
    }
    
    return OK;
}
