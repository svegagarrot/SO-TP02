// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"


extern int g_run_in_background;

static char **copy_args(int argc, char *argv[], int *ok) {
	char **process_argv = (char **) malloc(argc * sizeof(char *) + sizeof(char *));
	if (!process_argv) { *ok = 0; return NULL; }
	int i;
	for (i = 1; i < argc; i++) {
		process_argv[i - 1] = (char *) malloc(strlen(argv[i]) + 1);
		if (!process_argv[i - 1]) {
			for (int j = 0; j < i - 1; j++) free(process_argv[j]);
			free(process_argv);
			*ok = 0;
			return NULL;
		}
		int idx = 0;
		while (argv[i][idx] != '\0') {
			process_argv[i - 1][idx] = argv[i][idx];
			idx++;
		}
		process_argv[i - 1][idx] = '\0';
	}
	process_argv[argc - 1] = NULL;
	*ok = 1;
	return process_argv;
}

static void free_args(char **args) {
	if (!args) return;
	for (int i = 0; args[i] != NULL; i++) free(args[i]);
	free(args);
}

static int launch_test(const char *proc_name, void *entry, int argc, char *argv[]) {
	int ok;
	char **args = copy_args(argc, argv, &ok);
	if (!ok) { printf("Error: no se pudo asignar memoria para el proceso.\n"); return CMD_ERROR; }
	int is_fg = g_run_in_background ? 0 : 1;
	int64_t pid = execute_external_command(proc_name, entry, args, is_fg, 0, 0);
	if (pid <= 0) { free_args(args); printf("Error: no se pudo crear el proceso %s.\n", proc_name); return CMD_ERROR; }
	if (g_run_in_background) printf("Test %s iniciado con PID: %lld (background)\n", proc_name, pid);
	free_args(args);
	return OK;
}

int testMMCmd(int argc, char *argv[]) {
	if (argc != 2) { printf("Uso: testmm <max_mem> [&]\n"); return CMD_ERROR; }
	return launch_test("test_mm", test_mm_process_wrapper, argc, argv);
}

int testSyncCmd(int argc, char *argv[]) {
	if (argc < 2 || argc > 3) {
		printf("Uso: test_sync <repeticiones> [num_pares] [&]\n");
		printf("  repeticiones: número de iteraciones por proceso\n");
		printf("  num_pares: número de pares de procesos (opcional, default=2)\n");
		return CMD_ERROR;
	}
	return launch_test("test_synchro", test_sync_wrapper, argc, argv);
}

int testNoSynchroCmd(int argc, char *argv[]) {
	if (argc < 2 || argc > 3) {
		printf("Uso: test_no_synchro <repeticiones> [num_pares] [&]\n");
		printf("  repeticiones: número de iteraciones por proceso\n");
		printf("  num_pares: número de pares de procesos (opcional, default=2)\n");
		return CMD_ERROR;
	}
	return launch_test("test_no_synchro", test_no_synchro_wrapper, argc, argv);
}

int testProcesesCmd(int argc, char *argv[]) {
	if (argc != 2) { printf("Uso: test_proceses <max_proceses> [&]\n"); return CMD_ERROR; }
	return launch_test("test_processes", test_processes_wrapper, argc, argv);
}

int testPriorityCmd(int argc, char *argv[]) {
	if (argc != 2) { printf("Uso: test_priority <max_value> [&]\n"); return CMD_ERROR; }
	return launch_test("test_prio", test_prio_wrapper, argc, argv);
}
