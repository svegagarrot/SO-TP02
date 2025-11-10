// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"

extern int64_t test_mm(uint64_t argc, char *argv[]);
extern int64_t test_processes(uint64_t argc, char *argv[]);
extern uint64_t test_prio(uint64_t argc, char *argv[]);
extern uint64_t test_sync(uint64_t argc, char *argv[]);

#define STDIN_FD 0
#define STDOUT_FD 1

const char *state_to_string(int state) {
	switch (state) {
		case PROCESS_STATE_NEW:
			return "NEW";
		case PROCESS_STATE_READY:
			return "READY";
		case PROCESS_STATE_RUNNING:
			return "RUNNING";
		case PROCESS_STATE_BLOCKED:
			return "BLOCKED";
		case PROCESS_STATE_FINISHED:
			return "FINISHED";
		default:
			return "UNKNOWN";
	}
}

// Wrapper genérico para tests con un argumento
static void test_single_arg_wrapper(void *arg, int64_t (*test_fn)(uint64_t, char **)) {
	char **argv = (char **) arg;
	if (argv && argv[0]) {
		char *args[2];
		args[0] = argv[0];
		args[1] = NULL;
		test_fn(1, args);
		free(argv[0]);
		free(argv);
	}
}

// Wrapper genérico para tests de sincronización
static void test_sync_wrapper_common(void *arg, const char *use_sem) {
	char **argv = (char **) arg;
	if (argv && argv[0]) {
		char *args[3];
		args[0] = argv[0];
		args[1] = (char *)use_sem;
		if (argv[1]) {
			args[2] = argv[1];
			test_sync(3, args);
		} else {
			test_sync(2, args);
		}
		free(argv[0]);
		if (argv[1]) free(argv[1]);
		free(argv);
	}
}

void test_mm_process_wrapper(void *arg) {
	test_single_arg_wrapper(arg, test_mm);
}

void test_processes_wrapper(void *arg) {
	test_single_arg_wrapper(arg, test_processes);
}

void test_prio_wrapper(void *arg) {
	test_single_arg_wrapper(arg, (int64_t (*)(uint64_t, char **))test_prio);
}

void test_sync_wrapper(void *arg) {
	test_sync_wrapper_common(arg, "1");
}

void test_no_synchro_wrapper(void *arg) {
	test_sync_wrapper_common(arg, "0");
}

void loop_process_entry(void *arg) {
	int seconds = 1; 
	char **argv = (char **) arg;

	if (argv != NULL && argv[0] != NULL) {
		seconds = atoi(argv[0]);
		if (seconds <= 0) {
			seconds = 1; 
		}
		free(argv[0]);
		free(argv);
	}

	int64_t pid = my_getpid();

	while (1) {
		printf("Hola! Soy el proceso con ID %lld\n", pid);
		sleep(seconds * 1000); 
	}
}

void ps_process_entry(void *arg) {
	(void) arg; 

	process_info_t processes[MAX_PROCESS_INFO];
	uint64_t count = list_processes(processes, MAX_PROCESS_INFO);

	if (count == 0) {
		printf("No hay procesos en el sistema.\n");
		return;
	}

	printf("\nPID\tNombre\t\t\tEstado\t\tPrioridad\tRSP\t\t\tRBP\t\t\tForeground\n");
	printf("-----------------------------------------------------------------------------------------------------------"
		   "----------------\n");

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

void cat_process_entry(void *arg) {
	(void) arg; 
	char buffer[256];
	int n;

	while ((n = sys_read(STDIN_FD, buffer, sizeof(buffer) - 1)) > 0) {
		buffer[n] = '\0';
		printf("%s", buffer);
	}
}

void wc_process_entry(void *arg) {
	(void) arg; 

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
}

void filter_process_entry(void *arg) {
	(void) arg; 

	char buffer[256];
	int n;

	while ((n = sys_read(STDIN_FD, buffer, sizeof(buffer))) > 0) {
		for (int i = 0; i < n; i++) {
			char c = buffer[i];
			if (c != 'a' && c != 'e' && c != 'i' && c != 'o' && c != 'u' && c != 'A' && c != 'E' && c != 'I' &&
				c != 'O' && c != 'U') {
				printf("%c", c);
			}
		}
	}
}

void mvar_writer_entry(void *arg) {
	char **argv = (char **) arg;
	char writer_id = argv[0][0];
	char sem_empty[32];
	char sem_full[32];
	char sem_mutex[32];
	uint64_t pipe_id;

	int i = 0;
	while (i < 31 && argv[1][i] != '\0') {
		sem_empty[i] = argv[1][i];
		i++;
	}
	sem_empty[i] = '\0';

	i = 0;
	while (i < 31 && argv[2][i] != '\0') {
		sem_full[i] = argv[2][i];
		i++;
	}
	sem_full[i] = '\0';

	i = 0;
	while (i < 31 && argv[3][i] != '\0') {
		sem_mutex[i] = argv[3][i];
		i++;
	}
	sem_mutex[i] = '\0';

	pipe_id = 0;
	i = 0;
	while (argv[4][i] != '\0') {
		pipe_id = pipe_id * 10 + (argv[4][i] - '0');
		i++;
	}

	free(argv[0]);
	free(argv[1]);
	free(argv[2]);
	free(argv[3]);
	free(argv[4]);
	free(argv);

	if (!pipe_open(pipe_id)) {
		printf("Escritor %c: no pudo abrir el pipe %llu\n", writer_id, pipe_id);
		return;
	}

	const int write_fd = 2;
	if (!pipe_dup(pipe_id, write_fd, 1)) {
		printf("Escritor %c: no pudo conectar el pipe\n", writer_id);
		return; 
	}

	while (1) {
		int random_wait = (writer_id * 13) % 500 + 100;
		sleep(random_wait);

		my_sem_wait(sem_empty);
		my_sem_wait(sem_mutex);

		int written = sys_write(write_fd, &writer_id, 1);
		if (written == 1) {
			my_sem_post(sem_mutex);
			my_sem_post(sem_full);
		}
		else {
			my_sem_post(sem_mutex);
			my_sem_post(sem_empty);
		}
	}
}

void mvar_reader_entry(void *arg) {
	char **argv = (char **) arg;
	int reader_id;
	char sem_empty[32];
	char sem_full[32];
	char sem_mutex[32];
	uint64_t pipe_id;

	reader_id = 0;
	int i = 0;
	while (argv[0][i] != '\0') {
		reader_id = reader_id * 10 + (argv[0][i] - '0');
		i++;
	}

	i = 0;
	while (i < 31 && argv[1][i] != '\0') {
		sem_empty[i] = argv[1][i];
		i++;
	}
	sem_empty[i] = '\0';

	i = 0;
	while (i < 31 && argv[2][i] != '\0') {
		sem_full[i] = argv[2][i];
		i++;
	}
	sem_full[i] = '\0';

	i = 0;
	while (i < 31 && argv[3][i] != '\0') {
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

	if (!pipe_open(pipe_id)) {
		printf("Lector %d: no pudo abrir el pipe %llu\n", reader_id, pipe_id);
		return;
	}

	const int read_fd = 2;
	if (!pipe_dup(pipe_id, read_fd, 0)) { 
		printf("Lector %d: no pudo conectar el pipe\n", reader_id);
		return; 
	}

	uint32_t reader_colors[] = {
		0xFF0000, // Rojo - Lector 0
		0x00FF00, // Verde - Lector 1
		0x0080FF, // Azul claro - Lector 2
		0xFFFF00, // Amarillo - Lector 3
		0xFF00FF, // Magenta - Lector 4
		0x00FFFF, // Cyan - Lector 5
	};
	int num_colors = 6;
	uint32_t my_color = reader_colors[reader_id % num_colors];

	while (1) {
		int random_wait = (reader_id * 17) % 500 + 100;
		sleep(random_wait);

		
		my_sem_wait(sem_full);
		my_sem_wait(sem_mutex);

		char read_value = '?';
		int bytes_read = sys_read(read_fd, &read_value, 1);

		if (bytes_read == 1) {
			sys_video_putChar(read_value, my_color, 0x000000);
		}
		my_sem_post(sem_mutex);

		my_sem_post(sem_empty);
	}
}

void mem_process_entry(void *arg) {
	(void) arg;

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

	if (info.total_bytes > 0) {
		uint64_t used_percent = (info.used_bytes * 100) / info.total_bytes;
		uint64_t free_percent = (info.free_bytes * 100) / info.total_bytes;
		printf("\nPorcentaje de uso:   %llu%%\n", used_percent);
		printf("Porcentaje libre:    %llu%%\n", free_percent);
	}

	printf("\n");
}

void mmtype_process_entry(void *arg) {
	(void) arg; 
	char buf[32] = {0};
	if (get_type_of_mm(buf, sizeof(buf))) {
		printf("Memory manager activo: %s\n", buf);
	}
	else {
		printf("No se pudo obtener el tipo de memory manager\n");
	}
}
