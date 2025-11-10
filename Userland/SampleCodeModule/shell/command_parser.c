// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"

static int find_command_index(const char *name);
static int contains_pipe_symbol(const char *input);
static int execute_pipeline(char *left_input, char *right_input);

// Función helper para obtener la función de entrada del proceso desde el índice del comando
void *get_process_entry_function(int cmd_idx) {
	if (cmd_idx < 0)
		return NULL;

	const char *name = shellCmds[cmd_idx].name;
	if (!name)
		return NULL;

	// Mapear nombre del comando a su función de entrada
	if (strcmp(name, "cat") == 0)
		return (void *) cat_process_entry;
	if (strcmp(name, "wc") == 0)
		return (void *) wc_process_entry;
	if (strcmp(name, "filter") == 0)
		return (void *) filter_process_entry;
	if (strcmp(name, "ps") == 0)
		return (void *) ps_process_entry;
	if (strcmp(name, "loop") == 0)
		return (void *) loop_process_entry;
	if (strcmp(name, "mem") == 0)
		return (void *) mem_process_entry;
	if (strcmp(name, "mmtype") == 0)
		return (void *) mmtype_process_entry;
	if (strcmp(name, "testmm") == 0)
		return (void *) test_mm_process_wrapper;
	if (strcmp(name, "test_proceses") == 0)
		return (void *) test_processes_wrapper;
	if (strcmp(name, "test_synchro") == 0)
		return (void *) test_sync_wrapper;
	if (strcmp(name, "test_no_synchro") == 0)
		return (void *) test_no_synchro_wrapper;
	if (strcmp(name, "test_priority") == 0)
		return (void *) test_prio_wrapper;

	return NULL;
}

// Función unificada para ejecutar comandos externos con o sin pipes
int64_t execute_external_command(const char *name, void *function, char *argv[], int is_foreground,
								 uint64_t stdin_pipe_id, uint64_t stdout_pipe_id) {
	if (!name || !function) {
		return -1;
	}

	int64_t pid;

	// Si hay pipes, usar my_create_process_with_pipes, sino usar my_create_process
	if (stdin_pipe_id != 0 || stdout_pipe_id != 0) {
		pid = my_create_process_with_pipes((char *) name, function, argv, 1, is_foreground, stdin_pipe_id,
										   stdout_pipe_id);
	}
	else {
		pid = my_create_process((char *) name, function, argv, 1, is_foreground);
	}

	if (pid <= 0) {
		return -1;
	}

	// Si está en foreground, esperar a que termine
	if (is_foreground) {
		my_wait(pid);
	}

	return pid;
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

	// Obtener funciones de entrada de los procesos
	void *left_function = get_process_entry_function(left_idx);
	void *right_function = get_process_entry_function(right_idx);

	if (!left_function || !right_function) {
		printf("Error: no se pudo obtener la funcion de entrada del proceso.\n");
		return CMD_ERROR;
	}

	// Preparar argumentos para el comando izquierdo (sin el nombre del comando)
	char **left_process_argv = NULL;
	if (left_argc > 1) {
		left_process_argv = (char **) malloc((left_argc) * sizeof(char *));
		if (left_process_argv) {
			for (int i = 1; i < left_argc; i++) {
				int len = strlen(left_args[i]) + 1;
				left_process_argv[i - 1] = (char *) malloc(len);
				if (left_process_argv[i - 1]) {
					strncpy(left_process_argv[i - 1], left_args[i], len);
				}
			}
			left_process_argv[left_argc - 1] = NULL;
		}
	}

	// Preparar argumentos para el comando derecho (sin el nombre del comando)
	char **right_process_argv = NULL;
	if (right_argc > 1) {
		right_process_argv = (char **) malloc((right_argc) * sizeof(char *));
		if (right_process_argv) {
			for (int i = 1; i < right_argc; i++) {
				int len = strlen(right_args[i]) + 1;
				right_process_argv[i - 1] = (char *) malloc(len);
				if (right_process_argv[i - 1]) {
					strncpy(right_process_argv[i - 1], right_args[i], len);
				}
			}
			right_process_argv[right_argc - 1] = NULL;
		}
	}

	// Ejecutar comando izquierdo con stdout redirigido al pipe
	// El proceso izquierdo (escritor) se ejecuta en background
	int64_t left_pid =
		execute_external_command(shellCmds[left_idx].name, left_function, left_process_argv, 0, 0, pipe_id);

	int status = CMD_ERROR;
	int64_t right_pid = -1;

	if (left_pid > 0) {
		// Ejecutar comando derecho con stdin redirigido desde el pipe
		// El proceso derecho (lector) se ejecuta en FOREGROUND para que Ctrl+C funcione
		right_pid =
			execute_external_command(shellCmds[right_idx].name, right_function, right_process_argv, 1, pipe_id, 0);

		if (right_pid > 0) {
			status = OK;
		}
		else {
			printf("Error: el comando '%s' no puede usarse en un pipe.\n", shellCmds[right_idx].name);
			// Limpiar proceso izquierdo si falló el derecho
			// left_pid > 0 siempre es verdadero aquí porque estamos dentro del bloque if (left_pid > 0)
			my_kill(left_pid);
		}
	}
	else {
		printf("Error: el comando '%s' no puede usarse en un pipe.\n", shellCmds[left_idx].name);
	}

	// El proceso derecho (foreground) ya fue esperado por execute_external_command
	// Ahora debemos limpiar el proceso izquierdo
	if (status == OK && left_pid > 0) {
		// Dar un momento para que el proceso izquierdo detecte que el pipe se cerró
		// y termine naturalmente
		my_yield();

		// Si el proceso izquierdo todavía está vivo, matarlo
		// (esto pasa si el proceso tiene un loop infinito y no verifica errores de write)
		my_kill(left_pid);
	}

	// Liberar argumentos copiados DESPUÉS de que los procesos terminen
	if (left_process_argv) {
		for (int i = 0; left_process_argv[i] != NULL; i++) {
			free(left_process_argv[i]);
		}
		free(left_process_argv);
	}
	if (right_process_argv) {
		for (int i = 0; right_process_argv[i] != NULL; i++) {
			free(right_process_argv[i]);
		}
		free(right_process_argv);
	}

	return status;
}

int CommandParse(char *commandInput) {
	if (commandInput == NULL)
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

	if (argc == 0)
		return ERROR;

	// Detectar si el último argumento es "&" para ejecutar en background
	int is_background = 0;
	if (argc > 0 && strcmp(args[argc - 1], "&") == 0) {
		is_background = 1;
		argc--; // Remover el "&" de los argumentos
		if (argc == 0) {
			return ERROR; // Solo había "&", comando inválido
		}
	}

	for (int i = 0; shellCmds[i].name; i++) {
		if (strcmp(args[0], shellCmds[i].name) == 0) {
			// Pasar información de background a través de una variable global temporal
			extern int g_run_in_background;
			g_run_in_background = is_background;

			// Para comandos built-in, ejecutar directamente
			// Para comandos externos, la función del comando debe crear el proceso
			int result = shellCmds[i].function(argc, args);

			g_run_in_background = 0; // Reset
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
		}
		else if (!inArg) {
			args[argc++] = &input[i];
			inArg = 1;
		}
	}

	return argc;
}
