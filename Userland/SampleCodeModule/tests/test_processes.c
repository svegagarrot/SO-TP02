// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "lib.h"
#include "test_util.h"

enum State { RUNNING, BLOCKED, KILLED };

typedef struct P_rq {
	int32_t pid;
	enum State state;
} p_rq;

int64_t test_processes(uint64_t argc, char *argv[]) {
	uint8_t rq;
	uint8_t alive = 0;
	uint8_t action;
	uint64_t max_processes;
	char *argvAux[] = {0};

	if (argc != 1)
		return -1;

	if ((max_processes = satoi(argv[0])) <= 0)
		return -1;

	printf("=== TEST DE PROCESOS INICIADO ===\n");
	printf("Numero maximo de procesos: %llu\n", (unsigned long long) max_processes);

	p_rq p_rqs[max_processes];

	while (1) {
		printf("\n--- CICLO DE CREACION DE PROCESOS ---\n");

		// Create max_processes processes
		for (rq = 0; rq < max_processes; rq++) {
			printf("Creando proceso %d... ", rq + 1);
			p_rqs[rq].pid = my_create_process("endless_loop", endless_loop, argvAux, 1, 0); // background

			if (p_rqs[rq].pid == -1) {
				printf("ERROR\n");
				printf("test_processes: ERROR creating process\n");
				return -1;
			}
			else {
				printf("PID: %d\n", p_rqs[rq].pid);
				p_rqs[rq].state = RUNNING;
				alive++;
			}
		}

		printf("Total de procesos creados: %d\n", alive);

		// Randomly kills, blocks or unblocks processes until every one has been killed
		printf("\n--- CICLO DE MANIPULACION DE PROCESOS ---\n");
		while (alive > 0) {
			printf("Procesos vivos: %d\n", alive);

			for (rq = 0; rq < max_processes; rq++) {
				action = GetUniform(100) % 2;

				switch (action) {
					case 0:
						if (p_rqs[rq].state == RUNNING || p_rqs[rq].state == BLOCKED) {
							printf("Matando proceso PID %d... ", p_rqs[rq].pid);
							if (my_kill(p_rqs[rq].pid) == -1) {
								printf("ERROR\n");
								printf("test_processes: ERROR killing process\n");
								return -1;
							}
							printf("OK\n");
							p_rqs[rq].state = KILLED;
							alive--;
						}
						break;

					case 1:
						if (p_rqs[rq].state == RUNNING) {
							printf("Bloqueando proceso PID %d... ", p_rqs[rq].pid);
							if (my_block(p_rqs[rq].pid) == -1) {
								printf("ERROR\n");
								printf("test_processes: ERROR blocking process\n");
								return -1;
							}
							printf("OK\n");
							p_rqs[rq].state = BLOCKED;
						}
						break;
				}
			}

			// Randomly unblocks processes
			for (rq = 0; rq < max_processes; rq++)
				if (p_rqs[rq].state == BLOCKED && GetUniform(100) % 2) {
					printf("Desbloqueando proceso PID %d... ", p_rqs[rq].pid);
					if (my_unblock(p_rqs[rq].pid) == -1) {
						printf("ERROR\n");
						printf("test_processes: ERROR unblocking process\n");
						return -1;
					}
					printf("OK\n");
					p_rqs[rq].state = RUNNING;
				}
		}

		printf("\n--- CICLO COMPLETADO ---\n");
		printf("Todos los procesos fueron terminados. Reiniciando...\n");
		alive = 0; // Reset para el próximo ciclo
	}
}