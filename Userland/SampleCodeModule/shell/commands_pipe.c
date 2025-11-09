#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"

extern int g_run_in_background;

// cat: Imprime stdin tal como lo recibe
int catCmd(int argc, char *argv[]) {
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("cat", cat_process_entry, NULL, is_foreground, 0, 0);
    
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso cat.\n");
        return CMD_ERROR;
    }
    
    return OK;
}

// wc: Cuenta la cantidad de líneas del input
int wcCmd(int argc, char *argv[]) {
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("wc", wc_process_entry, NULL, is_foreground, 0, 0);
    
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso wc.\n");
        return CMD_ERROR;
    }
    
    return OK;
}

// filter: Filtra las vocales del input
int filterCmd(int argc, char *argv[]) {
    int is_foreground = g_run_in_background ? 0 : 1;
    int64_t pid = execute_external_command("filter", filter_process_entry, NULL, is_foreground, 0, 0);
    
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso filter.\n");
        return CMD_ERROR;
    }
    
    return OK;
}

// mvar: Simula MVar con múltiples lectores y escritores
int mvarCmd(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: mvar <num_escritores> <num_lectores>\n");
        printf("  num_escritores: cantidad de procesos escritores (1-26)\n");
        printf("  num_lectores: cantidad de procesos lectores (1-20)\n");
        return CMD_ERROR;
    }
    
    int num_writers = atoi(argv[1]);
    int num_readers = atoi(argv[2]);
    
    if (num_writers <= 0 || num_writers > 26) {
        printf("Error: num_escritores debe estar entre 1 y 26.\n");
        return CMD_ERROR;
    }
    
    if (num_readers <= 0 || num_readers > 20) {
        printf("Error: num_lectores debe estar entre 1 y 20.\n");
        return CMD_ERROR;
    }
    
    // Crear nombres únicos para los semáforos usando el PID actual
    int64_t current_pid = my_getpid();
    char sem_empty[32];
    char sem_full[32];
    char sem_mutex[32];
    
    sprintf(sem_empty, "mvar_e_%lld", current_pid);
    sprintf(sem_full, "mvar_f_%lld", current_pid);
    sprintf(sem_mutex, "mvar_m_%lld", current_pid);
    
    // Crear semáforos
    // sem_empty: inicialmente 1 (variable vacía, escritores pueden escribir)
    // sem_full: inicialmente 0 (no hay valor, lectores deben esperar)
    // sem_mutex: inicialmente 1 (mutex libre)
    if (my_sem_open(sem_empty, 1) < 0) {
        printf("Error: no se pudo crear el semaforo empty.\n");
        return CMD_ERROR;
    }
    
    if (my_sem_open(sem_full, 0) < 0) {
        printf("Error: no se pudo crear el semaforo full.\n");
        my_sem_close(sem_empty);
        return CMD_ERROR;
    }
    
    if (my_sem_open(sem_mutex, 1) < 0) {
        printf("Error: no se pudo crear el semaforo mutex.\n");
        my_sem_close(sem_empty);
        my_sem_close(sem_full);
        return CMD_ERROR;
    }
    
    // Crear pipe para comunicación entre escritores y lectores
    uint64_t pipe_id = pipe_create();
    if (pipe_id == 0) {
        printf("Error: no se pudo crear el pipe.\n");
        my_sem_close(sem_empty);
        my_sem_close(sem_full);
        my_sem_close(sem_mutex);
        return CMD_ERROR;
    }
    
    printf("Iniciando simulacion de MVar con %d escritores y %d lectores...\n", 
           num_writers, num_readers);
    printf("Pipe ID: %llu\n", pipe_id);
    
    // Crear procesos escritores
    for (int i = 0; i < num_writers; i++) {
        char **process_argv = (char **)malloc(6 * sizeof(char *));
        if (process_argv == NULL) {
            printf("Error: no se pudo asignar memoria para escritor %d.\n", i);
            continue;
        }
        
        // Argumento 0: ID del escritor
        process_argv[0] = (char *)malloc(2);
        if (process_argv[0] == NULL) {
            free(process_argv);
            continue;
        }
        process_argv[0][0] = 'A' + i;
        process_argv[0][1] = '\0';
        
        // Argumento 1: sem_empty
        int len = strlen(sem_empty);
        process_argv[1] = (char *)malloc(len + 1);
        if (process_argv[1] == NULL) {
            free(process_argv[0]);
            free(process_argv);
            continue;
        }
        for (int j = 0; j <= len; j++) process_argv[1][j] = sem_empty[j];
        
        // Argumento 2: sem_full
        len = strlen(sem_full);
        process_argv[2] = (char *)malloc(len + 1);
        if (process_argv[2] == NULL) {
            free(process_argv[0]);
            free(process_argv[1]);
            free(process_argv);
            continue;
        }
        for (int j = 0; j <= len; j++) process_argv[2][j] = sem_full[j];
        
        // Argumento 3: sem_mutex
        len = strlen(sem_mutex);
        process_argv[3] = (char *)malloc(len + 1);
        if (process_argv[3] == NULL) {
            free(process_argv[0]);
            free(process_argv[1]);
            free(process_argv[2]);
            free(process_argv);
            continue;
        }
        for (int j = 0; j <= len; j++) process_argv[3][j] = sem_mutex[j];
        
        // Argumento 4: pipe_id (como string)
        process_argv[4] = (char *)malloc(32);
        if (process_argv[4] == NULL) {
            free(process_argv[0]);
            free(process_argv[1]);
            free(process_argv[2]);
            free(process_argv[3]);
            free(process_argv);
            continue;
        }
        sprintf(process_argv[4], "%llu", pipe_id);
        
        process_argv[5] = NULL;
        
        char name[32];
        sprintf(name, "writer_%c", 'A' + i);
        
        int64_t pid = my_create_process(name, mvar_writer_entry, process_argv, 1, 0);
        if (pid <= 0) {
            printf("Error: no se pudo crear el escritor %c.\n", 'A' + i);
            for (int j = 0; j < 5; j++) free(process_argv[j]);
            free(process_argv);
        }
    }
    
    // Crear procesos lectores
    for (int i = 0; i < num_readers; i++) {
        char **process_argv = (char **)malloc(6 * sizeof(char *));
        if (process_argv == NULL) {
            printf("Error: no se pudo asignar memoria para lector %d.\n", i);
            continue;
        }
        
        // Argumento 0: ID del lector
        process_argv[0] = (char *)malloc(12);
        if (process_argv[0] == NULL) {
            free(process_argv);
            continue;
        }
        sprintf(process_argv[0], "%d", i);
        
        // Argumento 1: sem_empty
        int len = strlen(sem_empty);
        process_argv[1] = (char *)malloc(len + 1);
        if (process_argv[1] == NULL) {
            free(process_argv[0]);
            free(process_argv);
            continue;
        }
        for (int j = 0; j <= len; j++) process_argv[1][j] = sem_empty[j];
        
        // Argumento 2: sem_full
        len = strlen(sem_full);
        process_argv[2] = (char *)malloc(len + 1);
        if (process_argv[2] == NULL) {
            free(process_argv[0]);
            free(process_argv[1]);
            free(process_argv);
            continue;
        }
        for (int j = 0; j <= len; j++) process_argv[2][j] = sem_full[j];
        
        // Argumento 3: sem_mutex
        len = strlen(sem_mutex);
        process_argv[3] = (char *)malloc(len + 1);
        if (process_argv[3] == NULL) {
            free(process_argv[0]);
            free(process_argv[1]);
            free(process_argv[2]);
            free(process_argv);
            continue;
        }
        for (int j = 0; j <= len; j++) process_argv[3][j] = sem_mutex[j];
        
        // Argumento 4: pipe_id (como string)
        process_argv[4] = (char *)malloc(32);
        if (process_argv[4] == NULL) {
            free(process_argv[0]);
            free(process_argv[1]);
            free(process_argv[2]);
            free(process_argv[3]);
            free(process_argv);
            continue;
        }
        sprintf(process_argv[4], "%llu", pipe_id);
        
        process_argv[5] = NULL;
        
        char name[32];
        sprintf(name, "reader_%d", i);
        
        int64_t pid = my_create_process(name, mvar_reader_entry, process_argv, 1, 0);
        if (pid <= 0) {
            printf("Error: no se pudo crear el lector %d.\n", i);
            for (int j = 0; j < 5; j++) free(process_argv[j]);
            free(process_argv);
        }
    }
    
    printf("Todos los procesos lectores y escritores han sido creados.\n");
    printf("El proceso principal termina. Los procesos continuaran ejecutandose en background.\n");
    
    return OK;
}
