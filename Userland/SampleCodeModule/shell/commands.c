#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"
#include "../include/game.h"

int g_run_in_background = 0;

uint64_t g_current_pipe_id = 0;
int g_pipe_mode = -1;  // -1: sin pipe, 0: lectura (stdin), 1: escritura (stdout)

#define STDIN_FD 0
#define STDOUT_FD 1

static int find_command_index(const char *name);
static int contains_pipe_symbol(const char *input);
static int execute_pipeline(char *left_input, char *right_input);

extern void _invalidOp();
extern int64_t test_mm(uint64_t argc, char *argv[]);
extern int64_t test_processes(uint64_t argc, char *argv[]);
extern uint64_t test_prio(uint64_t argc, char *argv[]);
extern uint64_t test_sync(uint64_t argc, char *argv[]);
extern int mmTypeCmd(int argc, char *argv[]);

// Wrapper para test_mm que se puede ejecutar como proceso
static void test_mm_process_wrapper(void *arg) {
    char **argv = (char **)arg;
    if (argv != NULL && argv[0] != NULL) {
        char *args[2];
        args[0] = argv[0];
        args[1] = NULL;
        test_mm(1, args);
        // Liberar memoria
        free(argv[0]);
        free(argv);
    }
}

// Wrapper para test_processes que se puede ejecutar como proceso
static void test_processes_wrapper(void *arg) {
    char **argv = (char **)arg;
    if (argv != NULL && argv[0] != NULL) {
        char *args[2];
        args[0] = argv[0];
        args[1] = NULL;
        test_processes(1, args);
        // Liberar memoria
        free(argv[0]);
        free(argv);
    }
}

// Wrapper para test_prio que se puede ejecutar como proceso
static void test_prio_wrapper(void *arg) {
    char **argv = (char **)arg;
    if (argv != NULL && argv[0] != NULL) {
        char *args[2];
        args[0] = argv[0];
        args[1] = NULL;
        test_prio(1, args);
        // Liberar memoria
        free(argv[0]);
        free(argv);
    }
}

// Wrapper para test_sync que se puede ejecutar como proceso
static void test_sync_wrapper(void *arg) {
    char **argv = (char **)arg;
    if (argv != NULL && argv[0] != NULL) {
        // argv[0] = repeticiones, argv[1] = num_pares (puede ser NULL)
        char *args[3];
        args[0] = argv[0];
        args[1] = "1"; // use_sem = 1 para sync
        if (argv[1] != NULL) {
            args[2] = argv[1];
            test_sync(3, args);
        } else {
            test_sync(2, args);
        }
        // Liberar memoria
        free(argv[0]);
        if (argv[1]) free(argv[1]);
        free(argv);
    }
}

// Helper para convertir estado a string
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

// Wrapper para loop que se ejecuta como proceso
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

// Wrapper para ps que se ejecuta como proceso
static void ps_process_entry(void *arg) {
    (void)arg;  // No se necesitan argumentos
    
    process_info_t processes[MAX_PROCESS_INFO];
    uint64_t count = list_processes(processes, MAX_PROCESS_INFO);

    if (count == 0) {
        printf("No hay procesos en el sistema.\n");
        return;
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
}

// Wrapper para cat que se ejecuta como proceso
static void cat_process_entry(void *arg) {
    (void)arg;  // No se necesitan argumentos
    
    // Si hay un pipe configurado, conectarlo
    if (g_current_pipe_id != 0 && g_pipe_mode == 1) {
        // Modo escritura: redirigir stdout al pipe
        pipe_open(g_current_pipe_id);
        pipe_dup(g_current_pipe_id, STDOUT_FD, 1);
    }
    
    char buffer[256];
    int n;
    
    while ((n = sys_read(STDIN_FD, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }
    
    // Liberar el pipe si fue usado
    if (g_current_pipe_id != 0 && g_pipe_mode == 1) {
        pipe_release_fd(STDOUT_FD);
        pipe_close(g_current_pipe_id);
    }
}

// Wrapper para wc que se ejecuta como proceso
static void wc_process_entry(void *arg) {
    (void)arg;  // No se necesitan argumentos
    
    // Si hay un pipe configurado, conectarlo
    if (g_current_pipe_id != 0 && g_pipe_mode == 0) {
        // Modo lectura: redirigir stdin desde el pipe
        pipe_open(g_current_pipe_id);
        pipe_dup(g_current_pipe_id, STDIN_FD, 0);
    }
    
    char buffer[256];
    int n;
    int line_count = 0;
    
    while ((n = sys_read(STDIN_FD, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < n; i++) {
            if (buffer[i] == '\n') {
                line_count++;
            }
        }
    }
    
    printf("%d\n", line_count);
    
    // Liberar el pipe si fue usado
    if (g_current_pipe_id != 0 && g_pipe_mode == 0) {
        pipe_release_fd(STDIN_FD);
        pipe_close(g_current_pipe_id);
    }
}

// Wrapper para filter que filtra vocales del input
static void filter_process_entry(void *arg) {
    (void)arg;  // No se necesitan argumentos
    
    // Si hay un pipe configurado, conectarlo
    if (g_current_pipe_id != 0) {
        pipe_open(g_current_pipe_id);
        if (g_pipe_mode == 0) {
            // Modo lectura: redirigir stdin desde el pipe
            pipe_dup(g_current_pipe_id, STDIN_FD, 0);
        } else if (g_pipe_mode == 1) {
            // Modo escritura: redirigir stdout al pipe
            pipe_dup(g_current_pipe_id, STDOUT_FD, 1);
        }
    }
    
    char buffer[256];
    int n;
    
    while ((n = sys_read(STDIN_FD, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < n; i++) {
            char c = buffer[i];
            // Filtrar vocales (mayúsculas y minúsculas)
            if (c != 'a' && c != 'e' && c != 'i' && c != 'o' && c != 'u' &&
                c != 'A' && c != 'E' && c != 'I' && c != 'O' && c != 'U') {
                printf("%c", c);
            }
        }
    }
    
    // Liberar el pipe si fue usado
    if (g_current_pipe_id != 0) {
        if (g_pipe_mode == 0) {
            pipe_release_fd(STDIN_FD);
        } else if (g_pipe_mode == 1) {
            pipe_release_fd(STDOUT_FD);
        }
        pipe_close(g_current_pipe_id);
    }
}

// Wrapper para echo que imprime sus argumentos
static void echo_process_entry(void *arg) {
    char **argv = (char **)arg;
    
    if (argv != NULL && argv[0] != NULL) {
        int i = 0;
        while (argv[i] != NULL) {
            printf("%s", argv[i]);
            if (argv[i + 1] != NULL) {
                printf(" ");
            }
            free(argv[i]);
            i++;
        }
        printf("\n");
        free(argv);
    }
}



// Wrapper para escritores
static void mvar_writer_entry(void *arg) {
    char **argv = (char **)arg;
    char writer_id = argv[0][0]; 
    char sem_empty[32];
    char sem_full[32];
    char sem_mutex[32];
    uint64_t pipe_id;
    
    int i = 0;
    while (argv[1][i] != '\0' && i < 31) {
        sem_empty[i] = argv[1][i];
        i++;
    }
    sem_empty[i] = '\0';
    
    i = 0;
    while (argv[2][i] != '\0' && i < 31) {
        sem_full[i] = argv[2][i];
        i++;
    }
    sem_full[i] = '\0';
    
    i = 0;
    while (argv[3][i] != '\0' && i < 31) {
        sem_mutex[i] = argv[3][i];
        i++;
    }
    sem_mutex[i] = '\0';
    
    // Parsear pipe_id
    pipe_id = 0;
    i = 0;
    while (argv[4][i] != '\0') {
        pipe_id = pipe_id * 10 + (argv[4][i] - '0');
        i++;
    }
    
    // Liberar memoria
    free(argv[0]);
    free(argv[1]);
    free(argv[2]);
    free(argv[3]);
    free(argv[4]);
    free(argv);
    
    // Primero, abrir el pipe para obtener acceso
    if (!pipe_open(pipe_id)) {
        printf("Escritor %c: no pudo abrir el pipe %llu\n", writer_id, pipe_id);
        return;
    }
    
    // Conectar el pipe a un FD libre dentro del rango permitido
    const int write_fd = 2;  
    if (!pipe_dup(pipe_id, write_fd, 1)) {  
        printf("Escritor %c: no pudo conectar el pipe\n", writer_id);
        return;  // Error al conectar el pipe
    }
    
    while (1) {
        // Espera activa aleatoria (simulada con sleep)
        int random_wait = (writer_id * 13) % 500 + 100;
        sleep(random_wait);
        
        // 1. Esperar a que la MVar esté VACÍA
        my_sem_wait(sem_empty);
        my_sem_wait(sem_mutex);
        
        // 2. Escribir el valor en la variable compartida (pipe)
        int written = sys_write(write_fd, &writer_id, 1);
        if (written == 1) {
            printf("Escritor %c escribio '%c'\n", writer_id, writer_id);
            my_sem_post(sem_mutex);
            // 4. Senalizar que hay un valor disponible
            my_sem_post(sem_full);
        } else {
            printf("Escritor %c: error al escribir en la MVar\n", writer_id);
            my_sem_post(sem_mutex);
            // No se escribio nada, volver a habilitar a los escritores
            my_sem_post(sem_empty);
        }
        // ===== FIN SECCIÓN CRÍTICA =====
    }
}

// Wrapper para lectores
static void mvar_reader_entry(void *arg) {
    char **argv = (char **)arg;
    int reader_id;
    char sem_empty[32];
    char sem_full[32];
    char sem_mutex[32];
    uint64_t pipe_id;
    
    // Parsear reader_id
    reader_id = 0;
    int i = 0;
    while (argv[0][i] != '\0') {
        reader_id = reader_id * 10 + (argv[0][i] - '0');
        i++;
    }
    
    // Copiar nombres de semáforos
    i = 0;
    while (argv[1][i] != '\0' && i < 31) {
        sem_empty[i] = argv[1][i];
        i++;
    }
    sem_empty[i] = '\0';
    
    i = 0;
    while (argv[2][i] != '\0' && i < 31) {
        sem_full[i] = argv[2][i];
        i++;
    }
    sem_full[i] = '\0';
    
    i = 0;
    while (argv[3][i] != '\0' && i < 31) {
        sem_mutex[i] = argv[3][i];
        i++;
    }
    sem_mutex[i] = '\0';
    
    // Parsear pipe_id
    pipe_id = 0;
    i = 0;
    while (argv[4][i] != '\0') {
        pipe_id = pipe_id * 10 + (argv[4][i] - '0');
        i++;
    }
    
    // Liberar memoria
    free(argv[0]);
    free(argv[1]);
    free(argv[2]);
    free(argv[3]);
    free(argv[4]);
    free(argv);
    
    // Primero, abrir el pipe para obtener acceso
    if (!pipe_open(pipe_id)) {
        printf("Lector %d: no pudo abrir el pipe %llu\n", reader_id, pipe_id);
        return;
    }
    
    // Conectar el pipe a un FD libre dentro del rango permitido
    const int read_fd = 2;
    if (!pipe_dup(pipe_id, read_fd, 0)) {  // mode=0 para lectura
        printf("Lector %d: no pudo conectar el pipe\n", reader_id);
        return;  // Error al conectar el pipe
    }
    
    // Loop infinito
    while (1) {
        // Espera activa aleatoria
        int random_wait = (reader_id * 17) % 500 + 100;
        sleep(random_wait);
        
        // ===== SECCIÓN CRÍTICA: LEER =====
        // 1. Esperar a que haya un valor disponible (MVar LLENA)
        my_sem_wait(sem_full);
        my_sem_wait(sem_mutex);
        
        // 2. Leer el valor de la variable compartida (pipe)
        char read_value = '?';
        int bytes_read = sys_read(read_fd, &read_value, 1);
        
        // 3. Imprimir el valor leido
        if (bytes_read == 1) {
            printf("Lector %d leyo '%c'\n", reader_id, read_value);
        } else {
            printf("Lector %d: error al leer la MVar\n", reader_id);
        }
        my_sem_post(sem_mutex);
        
        // 4. Senalizar que la MVar está VACÍA (lista para recibir nuevo valor)
        my_sem_post(sem_empty);
        // ===== FIN SECCIÓN CRÍTICA =====
    }
}


// Wrapper para mem que se ejecuta como proceso
static void mem_process_entry(void *arg) {
    (void)arg;  // No se necesitan argumentos
    
    memory_info_t info;
    if (memory_info(&info) == 0) {
        printf("Error: no se pudo obtener la informacion de memoria.\n");
        return;
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
}

// Wrapper para mmtype que se ejecuta como proceso
static void mmtype_process_entry(void *arg) {
    (void)arg;  // No se necesitan argumentos
    
    char buf[32] = {0};
    if (get_type_of_mm(buf, sizeof(buf))) {
        printf("Memory manager activo: %s\n", buf);
    } else {
        printf("No se pudo obtener el tipo de memory manager\n");
    }
}

const TShellCmd shellCmds[] = {
    {"help", helpCmd, ": Muestra los comandos disponibles\n", 1},               // built-in
    {"exit", exitCmd, ": Salir del shell\n", 1},                                // built-in
    {"set-user", setUserCmd, ": Setea el nombre de usuario, con un maximo de 10 caracteres\n", 1},  // built-in
    {"clear", clearCmd, ": Limpia la pantalla\n", 1},                           // built-in
    {"time", timeCmd, ": Muestra la hora actual\n", 1},                         // built-in
    {"font-size", fontSizeCmd, ": Cambia el tamanio de la fuente\n", 1},       // built-in
    {"testmm", testMMCmd, ": Ejecuta el stress test de memoria. Uso: testmm <max_mem>\n", 0},       // externo
    {"testproceses", testProcesesCmd, ": Ejecuta el stress test de procesos. Uso: testproceses <max_proceses>\n", 0},  // externo
    {"test_sync", testSyncCmd, ": Ejecuta test sincronizado con semaforos. Uso: test_sync <repeticiones>\n", 0},  // externo
    {"test_no_synchro", testNoSynchroCmd, ": Ejecuta test sin sincronizacion. Uso: test_no_synchro <repeticiones>\n", 0},  // externo
    {"test_priority", testPriorityCmd, ": Ejecuta el test de prioridades. Uso: test_priority <max_value>\n", 0},  // externo
    {"exceptions", exceptionCmd, ": Testear excepciones. Ingrese: exceptions [zero/invalidOpcode] para testear alguna operacion\n", 1},  // built-in
    {"jugar", gameCmd, ": Inicia el modo juego\n", 1},                          // built-in
    {"regs", regsCmd, ": Muestra los ultimos 18 registros de la CPU\n", 1},    // built-in
    {"mmtype", mmTypeCmd, ": Muestra el tipo de memory manager activo\n", 0},  // externo
    {"ps", psCmd, ": Lista todos los procesos con sus propiedades\n", 0},       // externo
    {"loop", loopCmd, ": Crea un proceso que imprime su ID con saludo cada X segundos. Uso: loop <segundos>\n", 0},  // externo
    {"kill", killCmd, ": Mata un proceso por PID. Uso: kill <pid>\n", 1},      // built-in
    {"nice", niceCmd, ": Cambia la prioridad de un proceso. Uso: nice <pid> <prioridad>\n", 1},  // built-in
    {"block", blockCmd, ": Cambia el estado de un proceso entre bloqueado y listo. Uso: block <pid>\n", 1},  // built-in
    {"mem", memCmd, ": Imprime el estado de la memoria\n", 0},                  // externo
    {"cat", catCmd, ": Imprime el stdin tal como lo recibe\n", 0},             // externo
    {"wc", wcCmd, ": Cuenta la cantidad de lineas del input\n", 0},            // externo
    {"echo", echoCmd, ": Imprime argumentos. Uso: echo <texto>\n", 0},         // externo
    {"filter", filterCmd, ": Filtra las vocales del input\n", 0},              // externo
    {"mvar", mvarCmd, ": Simula MVar con lectores/escritores. Uso: mvar <escritores> <lectores>\n", 0},  // externo
    {NULL, NULL, NULL, 0}, 
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

static int find_command_index(const char *name) {
    if (name == NULL) {
        return -1;
    }
    for (int i = 0; shellCmds[i].name; i++) {
        if (strcmp(name, shellCmds[i].name) == 0) {
            return i;
        }
    }
    return -1;
}

static int contains_pipe_symbol(const char *input) {
    if (!input) {
        return 0;
    }
    for (const char *it = input; *it; ++it) {
        if (*it == '|') {
            return 1;
        }
    }
    return 0;
}

static int execute_pipeline(char *left_input, char *right_input) {
    char *left_args[MAX_ARGS];
    char *right_args[MAX_ARGS];

    int left_argc = fillCommandAndArgs(left_args, left_input);
    int right_argc = fillCommandAndArgs(right_args, right_input);

    if (left_argc == 0 || right_argc == 0) {
        printf("Error: formato de pipe invalido.\n");
        return CMD_ERROR;
    }

    if ((left_argc > 0 && strcmp(left_args[left_argc - 1], "&") == 0) ||
        (right_argc > 0 && strcmp(right_args[right_argc - 1], "&") == 0)) {
        printf("Error: los pipes no soportan ejecucion en background.\n");
        return CMD_ERROR;
    }

    int left_idx = find_command_index(left_args[0]);
    int right_idx = find_command_index(right_args[0]);

    if (left_idx < 0 || right_idx < 0) {
        return ERROR;
    }

    if (shellCmds[left_idx].is_builtin || shellCmds[right_idx].is_builtin) {
        printf("Error: los pipes solo pueden usarse con comandos externos.\n");
        return CMD_ERROR;
    }

    uint64_t pipe_id = pipe_create();
    if (pipe_id == 0) {
        printf("Error: no se pudo crear el pipe.\n");
        return CMD_ERROR;
    }

    int original_background = g_run_in_background;
    int status = CMD_ERROR;
    int64_t left_pid = -1;
    int64_t right_pid = -1;

    // Configurar pipe para el comando izquierdo (escritura)
    g_current_pipe_id = pipe_id;
    g_pipe_mode = 1;  // Modo escritura (stdout)
    g_run_in_background = 1;
    
    reset_last_spawned_pid();
    int left_result = shellCmds[left_idx].function(left_argc, left_args);
    left_pid = get_last_spawned_pid();

    // Verificar si el comando izquierdo se ejecutó correctamente
    if (left_result != CMD_ERROR && left_pid > 0) {
        // Configurar pipe para el comando derecho (lectura)
        g_current_pipe_id = pipe_id;
        g_pipe_mode = 0;  // Modo lectura (stdin)
        
        reset_last_spawned_pid();
        int right_result = shellCmds[right_idx].function(right_argc, right_args);
        right_pid = get_last_spawned_pid();

        // Verificar si el comando derecho se ejecutó correctamente
        if (right_result != CMD_ERROR && right_pid > 0) {
            status = OK;
        } else {
            printf("Error: el comando '%s' no puede usarse en un pipe.\n", shellCmds[right_idx].name);
        }
    } else {
        printf("Error: el comando '%s' no puede usarse en un pipe.\n", shellCmds[left_idx].name);
    }

    // Limpiar variables globales
    g_current_pipe_id = 0;
    g_pipe_mode = -1;
    g_run_in_background = original_background;

    // Manejar procesos según el resultado
    if (left_pid > 0 && status == CMD_ERROR) {
        my_wait(left_pid);
    }

    if (status == OK && !original_background) {
        if (left_pid > 0) {
            my_wait(left_pid);
        }
        if (right_pid > 0) {
            my_wait(right_pid);
        }
    }

    return status;
}

int CommandParse(char *commandInput){
    if(commandInput == NULL)
        return ERROR;
    
    for (char *p = commandInput; *p; ++p) {
        if (*p == '|') {
            char *right_input = p + 1;
            *p = '\0';

            // Saltar espacios iniciales en la segunda parte
            while (*right_input == ' ') {
                right_input++;
            }

            if (contains_pipe_symbol(right_input)) {
                printf("Error: solo se admite un pipe por comando.\n");
                return CMD_ERROR;
            }

            return execute_pipeline(commandInput, right_input);
        }
    }

    char *args[MAX_ARGS];
    int argc = fillCommandAndArgs(args, commandInput);

    if(argc == 0)
        return ERROR;

    // Detectar si el último argumento es "&" para ejecutar en background
    int is_background = 0;
    if (argc > 0 && strcmp(args[argc - 1], "&") == 0) {
        is_background = 1;
        argc--;  // Remover el "&" de los argumentos
        if (argc == 0) {
            return ERROR;  // Solo había "&", comando inválido
        }
    }

    for(int i = 0; shellCmds[i].name; i++) {
        if(strcmp(args[0], shellCmds[i].name) == 0) {
            // Pasar información de background a través de una variable global temporal
            extern int g_run_in_background;
            g_run_in_background = is_background;
            
            // Para comandos built-in, ejecutar directamente
            // Para comandos externos, la función del comando debe crear el proceso
            int result = shellCmds[i].function(argc, args);
            
            g_run_in_background = 0;  // Reset
            return result;
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
    extern int g_run_in_background;
    
    if (argc != 2) {
        printf("Uso: testmm <max_mem> [&]\n");
        return CMD_ERROR;
    }

    // Si se ejecuta en background, crear un proceso
    if (g_run_in_background) {
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
        
        // Usar el wrapper en lugar de test_mm directamente
        int64_t pid = my_create_process("test_mm", test_mm_process_wrapper, process_argv, 1, 0);  // background
        if (pid <= 0) {
            free(arg_copy);
            free(process_argv);
            printf("Error: no se pudo crear el proceso test_mm.\n");
            return CMD_ERROR;
        }
        
        printf("Test de memoria iniciado con PID: %lld (background)\n", pid);
        return OK;
    } else {
        // Ejecutar directamente en foreground (modo actual)
        int64_t result = test_mm(argc - 1, argv + 1);
        if (result == -1) {
            printf("testmm fallo\n");
            return CMD_ERROR;
        }
        return OK;
    }
}

int testSyncCmd(int argc, char *argv[]) {
    extern int g_run_in_background;
    
    if (argc < 2 || argc > 3) {
        printf("Uso: test_sync <repeticiones> [num_pares] [&]\n");
        printf("  repeticiones: número de iteraciones por proceso\n");
        printf("  num_pares: número de pares de procesos (opcional, default=2)\n");
        return CMD_ERROR;
    }

    // Si se ejecuta en background, crear un proceso
    if (g_run_in_background) {
        // Necesitamos 2 o 3 argumentos: repeticiones, use_sem=1, [num_pares]
        int num_args = (argc == 3) ? 3 : 2;
        char **process_argv = (char **)malloc(num_args * sizeof(char *));
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
        
        int64_t pid = my_create_process("test_sync", test_sync_wrapper, process_argv, 1, 0);  // background
        if (pid <= 0) {
            free(arg1_copy);
            if (argc == 3) free(process_argv[1]);
            free(process_argv);
            printf("Error: no se pudo crear el proceso test_sync.\n");
            return CMD_ERROR;
        }
        
        printf("Test de sincronizacion iniciado con PID: %lld (background)\n", pid);
        return OK;
    } else {
        // Ejecutar directamente en foreground
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
}

int testNoSynchroCmd(int argc, char *argv[]) {
    // Este comando NO debe correr en background ya que es un test
    // Siempre lo ejecutamos directamente (foreground implícito)
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
    extern int g_run_in_background;
    
    if (argc != 2) {
        printf("Uso: testproceses <max_proceses> [&]\n");
        return CMD_ERROR;
    }

    // Si se ejecuta en background, crear un proceso
    if (g_run_in_background) {
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
        
        int64_t pid = my_create_process("test_processes", test_processes_wrapper, process_argv, 1, 0);  // background
        if (pid <= 0) {
            free(arg_copy);
            free(process_argv);
            printf("Error: no se pudo crear el proceso test_processes.\n");
            return CMD_ERROR;
        }
        
        printf("Test de procesos iniciado con PID: %lld (background)\n", pid);
        return OK;
    } else {
        // Ejecutar directamente en foreground
        int64_t result = test_processes(argc - 1, argv + 1);

        if (result == -1) {
            printf("testproceses fallo\n");
            return CMD_ERROR;
        }

        return OK;
    }
}

int gameCmd(int argc, char *argv[]) {
    game_main_screen();
    return OK;
}

int mmTypeCmd(int argc, char *argv[]) {
    extern int g_run_in_background;
    
    // Siempre crear como proceso separado
    int is_foreground = g_run_in_background ? 0 : 1;
    
    int64_t pid = my_create_process("mmtype", mmtype_process_entry, NULL, 1, is_foreground);
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso mmtype.\n");
        return CMD_ERROR;
    }
    
    if (!g_run_in_background) {
        // Foreground: esperar a que termine
        my_wait(pid);
    } else {
        printf("Proceso mmtype creado con PID: %lld (background)\n", pid);
    }
    
    return OK;
}

int testPriorityCmd(int argc, char *argv[]) {
    extern int g_run_in_background;
    
    if (argc != 2) {
        printf("Uso: test_priority <max_value> [&]\n");
        return CMD_ERROR;
    }
    
    // Si se ejecuta en background, crear un proceso
    if (g_run_in_background) {
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
        
        int64_t pid = my_create_process("test_prio", test_prio_wrapper, process_argv, 1, 0);  // background
        if (pid <= 0) {
            free(arg_copy);
            free(process_argv);
            printf("Error: no se pudo crear el proceso test_prio.\n");
            return CMD_ERROR;
        }
        
        printf("Test de prioridades iniciado con PID: %lld (background)\n", pid);
        return OK;
    } else {
        // Ejecutar directamente en foreground
        uint64_t res = test_prio(argc - 1, argv + 1);
        if ((int64_t)res == -1) {
            printf("test_priority fallo\n");
            return CMD_ERROR;
        }
        return OK;
    }
}

int psCmd(int argc, char *argv[]) {
    extern int g_run_in_background;
    
    // Siempre crear como proceso separado
    int is_foreground = g_run_in_background ? 0 : 1;
    
    int64_t pid = my_create_process("ps", ps_process_entry, NULL, 1, is_foreground);
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso ps.\n");
        return CMD_ERROR;
    }
    
    if (!g_run_in_background) {
        // Foreground: esperar a que termine
        my_wait(pid);
    } else {
        printf("Proceso ps creado con PID: %lld (background)\n", pid);
    }
    
    return OK;
}

int loopCmd(int argc, char *argv[]) {
    extern int g_run_in_background;
    
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

    // Si hay &, ejecutar en background. Si no, en foreground.
    int is_foreground = g_run_in_background ? 0 : 1;
    
    int64_t pid = my_create_process("loop_process", loop_process_entry, process_argv, 1, is_foreground);
    if (pid <= 0) {
        free(seconds_str);
        free(process_argv);
        printf("Error: no se pudo crear el proceso loop.\n");
        return CMD_ERROR;
    }

    printf("Proceso loop creado con PID: %lld%s\n", pid, g_run_in_background ? " (background)" : "");
    
    // Si NO es background, esperar a que termine el proceso
    if (!g_run_in_background) {
        my_wait(pid);
        printf("\nProceso loop terminado.\n");
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

int memCmd(int argc, char *argv[]) {
    extern int g_run_in_background;
    
    // Siempre crear como proceso separado
    int is_foreground = g_run_in_background ? 0 : 1;
    
    int64_t pid = my_create_process("mem", mem_process_entry, NULL, 1, is_foreground);
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso mem.\n");
        return CMD_ERROR;
    }
    
    if (!g_run_in_background) {
        // Foreground: esperar a que termine
        my_wait(pid);
    } else {
        printf("Proceso mem creado con PID: %lld (background)\n", pid);
    }
    
    return OK;
}

// cat: Imprime stdin tal como lo recibe
int catCmd(int argc, char *argv[]) {
    extern int g_run_in_background;
    
    // Siempre crear como proceso separado
    int is_foreground = g_run_in_background ? 0 : 1;
    
    int64_t pid = my_create_process("cat", cat_process_entry, NULL, 1, is_foreground);
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso cat.\n");
        return CMD_ERROR;
    }
    
    // Solo esperar y mostrar mensaje si NO estamos en background (no en pipe)
    if (!g_run_in_background) {
        my_wait(pid);
    }
    
    return OK;
}

// wc: Cuenta la cantidad de líneas del input
int wcCmd(int argc, char *argv[]) {
    extern int g_run_in_background;
    
    // Siempre crear como proceso separado
    int is_foreground = g_run_in_background ? 0 : 1;
    
    int64_t pid = my_create_process("wc", wc_process_entry, NULL, 1, is_foreground);
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso wc.\n");
        return CMD_ERROR;
    }
    
    // Solo esperar y mostrar mensaje si NO estamos en background (no en pipe)
    if (!g_run_in_background) {
        my_wait(pid);
    }
    
    return OK;
}

// echo: Imprime sus argumentos
int echoCmd(int argc, char *argv[]) {
    extern int g_run_in_background;
    
    if (argc < 2) {
        printf("\n");
        return OK;
    }
    
    // Copiar argumentos para pasarlos al proceso
    char **process_argv = (char **)malloc((argc) * sizeof(char *));
    if (process_argv == NULL) {
        printf("Error: no se pudo asignar memoria.\n");
        return CMD_ERROR;
    }
    
    for (int i = 1; i < argc; i++) {
        int len = 0;
        while (argv[i][len] != '\0') len++;
        
        process_argv[i-1] = (char *)malloc(len + 1);
        if (process_argv[i-1] == NULL) {
            for (int j = 0; j < i-1; j++) free(process_argv[j]);
            free(process_argv);
            printf("Error: no se pudo asignar memoria.\n");
            return CMD_ERROR;
        }
        
        for (int j = 0; j < len; j++) {
            process_argv[i-1][j] = argv[i][j];
        }
        process_argv[i-1][len] = '\0';
    }
    process_argv[argc-1] = NULL;
    
    int is_foreground = g_run_in_background ? 0 : 1;
    
    int64_t pid = my_create_process("echo", echo_process_entry, process_argv, 1, is_foreground);
    if (pid <= 0) {
        for (int i = 0; process_argv[i] != NULL; i++) free(process_argv[i]);
        free(process_argv);
        printf("Error: no se pudo crear el proceso echo.\n");
        return CMD_ERROR;
    }
    
    if (!g_run_in_background) {
        my_wait(pid);
    }
    
    return OK;
}

// filter: Filtra las vocales del input
int filterCmd(int argc, char *argv[]) {
    extern int g_run_in_background;
    
    // Siempre crear como proceso separado
    int is_foreground = g_run_in_background ? 0 : 1;
    
    int64_t pid = my_create_process("filter", filter_process_entry, NULL, 1, is_foreground);
    if (pid <= 0) {
        printf("Error: no se pudo crear el proceso filter.\n");
        return CMD_ERROR;
    }
    
    // Solo esperar y mostrar mensaje si NO estamos en background (no en pipe)
    if (!g_run_in_background) {
        my_wait(pid);
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
