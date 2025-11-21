// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/commands.h"
#include "../include/lib.h"
#include "../include/shell.h"
#include "../include/syscall.h"

extern int g_run_in_background;

int catCmd(int argc, char *argv[]) {
	int is_foreground = g_run_in_background ? 0 : 1;
	int64_t pid = execute_external_command("cat", cat_process_entry, NULL, is_foreground, 0, 0);

	if (pid <= 0) {
		printf("Error: no se pudo crear el proceso cat.\n");
		return CMD_ERROR;
	}

	return OK;
}

int wcCmd(int argc, char *argv[]) {
	int is_foreground = g_run_in_background ? 0 : 1;
	int64_t pid = execute_external_command("wc", wc_process_entry, NULL, is_foreground, 0, 0);

	if (pid <= 0) {
		printf("Error: no se pudo crear el proceso wc.\n");
		return CMD_ERROR;
	}

	return OK;
}

int filterCmd(int argc, char *argv[]) {
	int is_foreground = g_run_in_background ? 0 : 1;
	int64_t pid = execute_external_command("filter", filter_process_entry, NULL, is_foreground, 0, 0);

	if (pid <= 0) {
		printf("Error: no se pudo crear el proceso filter.\n");
		return CMD_ERROR;
	}

	return OK;
}

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

	int64_t current_pid = my_getpid();
	char suffix[32];
	char slots[32];
	char items[32];
	char mutex[32];

	sprintf(suffix, "%llx", current_pid);
	sprintf(slots, "mvar_slots_%s", suffix);
	sprintf(items, "mvar_items_%s", suffix);
	sprintf(mutex, "mvar_mutex_%s", suffix);


	if (my_sem_open(slots, 1) < 0) {
		printf("Error: no se pudo crear el semaforo slots.\n");
		return CMD_ERROR;
	}

	if (my_sem_open(items, 0) < 0) {
		printf("Error: no se pudo crear el semaforo items.\n");
		my_sem_close(slots);
		return CMD_ERROR;
	}

	if (my_sem_open(mutex, 1) < 0) {
		printf("Error: no se pudo crear el semaforo mutex.\n");
		my_sem_close(slots);
		my_sem_close(items);
		return CMD_ERROR;
	}

	uint64_t pipe_id = pipe_create();
	if (pipe_id == 0) {
		printf("Error: no se pudo crear el pipe.\n");
		my_sem_close(slots);
		my_sem_close(items);
		my_sem_close(mutex);
		return CMD_ERROR;
	}

	printf("Iniciando simulacion de MVar con %d escritores y %d lectores...\n", num_writers, num_readers);
	printf("Pipe ID: %llu\n", pipe_id);

	for (int i = 0; i < num_writers; i++) {
		char **process_argv = (char **) malloc(7 * sizeof(char *));
		if (process_argv == NULL) {
			printf("Error: no se pudo asignar memoria para escritor %d.\n", i);
			continue;
		}

		process_argv[0] = (char *) malloc(2);
		if (process_argv[0] == NULL) {
			free(process_argv);
			continue;
		}
		process_argv[0][0] = 'A' + (i % 26);
		process_argv[0][1] = '\0';

		int len = strlen(slots);
		process_argv[1] = (char *) malloc(len + 1);
		if (process_argv[1] == NULL) {
			free(process_argv[0]);
			free(process_argv);
			continue;
		}
		for (int j = 0; j <= len; j++)
			process_argv[1][j] = slots[j];

		len = strlen(items);
		process_argv[2] = (char *) malloc(len + 1);
		if (process_argv[2] == NULL) {
			free(process_argv[0]);
			free(process_argv[1]);
			free(process_argv);
			continue;
		}
		for (int j = 0; j <= len; j++)
			process_argv[2][j] = items[j];

		len = strlen(mutex);
		process_argv[3] = (char *) malloc(len + 1);
		if (process_argv[3] == NULL) {
			free(process_argv[0]);
			free(process_argv[1]);
			free(process_argv[2]);
			free(process_argv);
			continue;
		}
		for (int j = 0; j <= len; j++)
			process_argv[3][j] = mutex[j];

		process_argv[4] = (char *) malloc(32);
		if (process_argv[4] == NULL) {
			free(process_argv[0]);
			free(process_argv[1]);
			free(process_argv[2]);
			free(process_argv[3]);
			free(process_argv);
			continue;
		}
		sprintf(process_argv[4], "%llu", pipe_id);

		int delay = 2 + (i % 3);
		process_argv[5] = (char *) malloc(12);
		if (process_argv[5] == NULL) {
			free(process_argv[0]);
			free(process_argv[1]);
			free(process_argv[2]);
			free(process_argv[3]);
			free(process_argv[4]);
			free(process_argv);
			continue;
		}
		sprintf(process_argv[5], "%d", delay);

		process_argv[6] = NULL;

		char name[32];
		sprintf(name, "mvar_writer");

		int64_t pid = my_create_process(name, mvar_writer_entry, process_argv, 1, 0);
		if (pid <= 0) {
			printf("Error: no se pudo crear el escritor %c.\n", 'A' + (i % 26));
			for (int j = 0; j < 6; j++)
				free(process_argv[j]);
			free(process_argv);
		}
	}

	for (int j = 0; j < num_readers; j++) {
		char **process_argv = (char **) malloc(7 * sizeof(char *));
		if (process_argv == NULL) {
			printf("Error: no se pudo asignar memoria para lector %d.\n", j);
			continue;
		}

		process_argv[0] = (char *) malloc(12);
		if (process_argv[0] == NULL) {
			free(process_argv);
			continue;
		}
		sprintf(process_argv[0], "%d", j);

		int len = strlen(items);
		process_argv[1] = (char *) malloc(len + 1);
		if (process_argv[1] == NULL) {
			free(process_argv[0]);
			free(process_argv);
			continue;
		}
		for (int k = 0; k <= len; k++)
			process_argv[1][k] = items[k];

		len = strlen(slots);
		process_argv[2] = (char *) malloc(len + 1);
		if (process_argv[2] == NULL) {
			free(process_argv[0]);
			free(process_argv[1]);
			free(process_argv);
			continue;
		}
		for (int k = 0; k <= len; k++)
			process_argv[2][k] = slots[k];

		len = strlen(mutex);
		process_argv[3] = (char *) malloc(len + 1);
		if (process_argv[3] == NULL) {
			free(process_argv[0]);
			free(process_argv[1]);
			free(process_argv[2]);
			free(process_argv);
			continue;
		}
		for (int k = 0; k <= len; k++)
			process_argv[3][k] = mutex[k];

		process_argv[4] = (char *) malloc(32);
		if (process_argv[4] == NULL) {
			free(process_argv[0]);
			free(process_argv[1]);
			free(process_argv[2]);
			free(process_argv[3]);
			free(process_argv);
			continue;
		}
		sprintf(process_argv[4], "%llu", pipe_id);

		int delay = 3 + (j % 3);
		process_argv[5] = (char *) malloc(12);
		if (process_argv[5] == NULL) {
			free(process_argv[0]);
			free(process_argv[1]);
			free(process_argv[2]);
			free(process_argv[3]);
			free(process_argv[4]);
			free(process_argv);
			continue;
		}
		sprintf(process_argv[5], "%d", delay);

		process_argv[6] = NULL;

		char name[32];
		sprintf(name, "mvar_reader");

		int64_t pid = my_create_process(name, mvar_reader_entry, process_argv, 1, 0);
		if (pid <= 0) {
			printf("Error: no se pudo crear el lector %d.\n", j);
			for (int k = 0; k < 6; k++)
				free(process_argv[k]);
			free(process_argv);
		}
	}

	printf("Todos los procesos lectores y escritores han sido creados.\n");
	printf("El proceso principal termina. Los procesos continuaran ejecutandose en background.\n");

	return OK;
}
