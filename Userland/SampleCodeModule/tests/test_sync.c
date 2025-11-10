// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/lib.h"
#include "syscall.h"
#include "test_util.h"
#include <stdint.h>

#define SEM_ID "sem"
#define TOTAL_PAIR_PROCESSES 2

int64_t global; // shared memory

void slowInc(int64_t *p, int64_t inc) {
	uint64_t aux = *p;
	my_yield(); // This makes the race condition highly probable
	aux += inc;
	*p = aux;
}

/* Cuenta elementos de argv hasta NULL */
static uint64_t count_argv(char *argv[]) {
	uint64_t c = 0;
	while (argv && argv[c] != NULL)
		c++;
	return c;
}

uint64_t my_process_inc(char *argv[]) {
	uint64_t n;
	int8_t inc;
	int8_t use_sem;

	uint64_t argc = count_argv(argv);
	if (argc != 3)
		return (uint64_t) -1;

	if ((n = satoi(argv[0])) <= 0)
		return (uint64_t) -1;
	if ((inc = satoi(argv[1])) == 0)
		return (uint64_t) -1;
	if ((use_sem = satoi(argv[2])) < 0)
		return (uint64_t) -1;

	if (use_sem)
		if (!my_sem_open(SEM_ID, 1)) {
			printf("test_sync: ERROR opening semaphore\n");
			return -1;
		}

	uint64_t i;
	for (i = 0; i < n; i++) {
		if (use_sem)
			my_sem_wait(SEM_ID);
		slowInc(&global, inc);
		if (use_sem)
			my_sem_post(SEM_ID);
	}

	if (use_sem)
		my_sem_close(SEM_ID);

	return 0;
}

uint64_t test_sync(uint64_t argc, char *argv[]) { //{n, use_sem, 0}
	uint64_t pids[2 * TOTAL_PAIR_PROCESSES];

	if (argc != 2)
		return -1;

	char *argvDec[] = {argv[0], "-1", argv[1], NULL};
	char *argvInc[] = {argv[0], "1", argv[1], NULL};

	global = 0;

	uint64_t i;
	for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
		pids[i] = my_create_process("my_process_inc", NULL, argvDec, 1, 0);						   // background
		pids[i + TOTAL_PAIR_PROCESSES] = my_create_process("my_process_inc", NULL, argvInc, 1, 0); // background
	}

	for (i = 0; i < TOTAL_PAIR_PROCESSES; i++) {
		my_wait(pids[i]);
		my_wait(pids[i + TOTAL_PAIR_PROCESSES]);
	}

	printf("Final value: %lld\n", (long long) global);

	return 0;
}