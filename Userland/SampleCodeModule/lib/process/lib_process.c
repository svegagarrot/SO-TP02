// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../../include/lib.h"
#include "../../include/syscall.h"
#include <stddef.h>
#include <stdint.h>

static int64_t g_last_spawned_pid = -1;

extern int strcmp(const char *s1, const char *s2);
extern size_t strlen(const char *s);
extern void *malloc(size_t size);
extern void free(void *ptr);

static void *resolve_function_by_name(const char *name) {
	if (name == NULL)
		return NULL;
	
	void zero_to_max(void);
	void endless_loop(void);
	void endless_loop_print(uint64_t wait);
	uint64_t my_process_inc(char *argv[]);

	if (strcmp(name, "zero_to_max") == 0)
		return (void *) zero_to_max;
	if (strcmp(name, "endless_loop") == 0)
		return (void *) endless_loop;
	if (strcmp(name, "endless_loop_print") == 0)
		return (void *) endless_loop_print;
	if (strcmp(name, "my_process_inc") == 0)
		return (void *) my_process_inc;
	return NULL;
}

int64_t my_create_process(char *name, void *function, char *argv[], uint64_t priority, int is_foreground) {
	if (function == NULL || (uintptr_t) function < 0x10000) {
		function = resolve_function_by_name(name);
		if (function == NULL) {
			return -1; 
		}
	}
	int64_t pid = (int64_t) sys_create_process(name, function, argv, priority, (uint64_t) is_foreground);
	if (pid > 0) {
		g_last_spawned_pid = pid;
	}
	return pid;
}

int64_t my_create_process_with_pipes(char *name, void *function, char *argv[], uint64_t priority, int is_foreground,
									 uint64_t stdin_pipe_id, uint64_t stdout_pipe_id) {
	if (function == NULL || (uintptr_t) function < 0x10000) {
		function = resolve_function_by_name(name);
		if (function == NULL) {
			return -1; 
		}
	}

	uint64_t packed =
		(stdin_pipe_id & 0xFFFF) | ((stdout_pipe_id & 0xFFFF) << 16) | (((uint64_t) is_foreground & 0x1) << 32);

	if (stdin_pipe_id != 0 || stdout_pipe_id != 0) {
		// Marcar explícitamente que estamos usando el formato extendido (pipes)
		packed |= (1ULL << 33);
	}

	int64_t pid = (int64_t) sys_create_process(name, function, argv, priority, packed);
	if (pid > 0) {
		g_last_spawned_pid = pid;
	}
	return pid;
}

int64_t my_kill(uint64_t pid) {
	return sys_kill(pid);
}

int64_t my_block(uint64_t pid) {
	return sys_block(pid);
}

int64_t my_unblock(uint64_t pid) {
	return sys_unblock(pid);
}

int64_t my_getpid() {
	return (int64_t) sys_getpid();
}

int64_t my_nice(uint64_t pid, uint64_t newPrio) {
	return (int64_t) sys_set_priority(pid, newPrio);
}

int64_t my_yield() {
	return (int64_t) sys_yield();
}

int64_t my_wait(int64_t pid) {
	return (int64_t) sys_wait((uint64_t) pid);
}

uint64_t list_processes(process_info_t *buffer, uint64_t max_count) {
	if (!buffer || max_count == 0) {
		return 0;
	}
	return sys_list_processes(buffer, max_count);
}

void reset_last_spawned_pid(void) {
	g_last_spawned_pid = -1;
}

int64_t get_last_spawned_pid(void) {
	return g_last_spawned_pid;
}


#define MAX_USER_SEMS 8
static char *user_sem_names[MAX_USER_SEMS];
static uint64_t user_sem_ids[MAX_USER_SEMS];

static int find_sem_index(const char *name) {
	if (!name)
		return -1;
	for (int i = 0; i < MAX_USER_SEMS; ++i) {
		if (user_sem_names[i] && strcmp(user_sem_names[i], name) == 0)
			return i;
	}
	return -1;
}

static int find_free_sem_slot(void) {
	for (int i = 0; i < MAX_USER_SEMS; ++i)
		if (!user_sem_names[i])
			return i;
	return -1;
}

int64_t my_sem_open(char *sem_id, uint64_t initialValue) {
	if (!sem_id)
		return 0;
	int idx = find_sem_index(sem_id);
	if (idx >= 0) {
		sys_sem_open(user_sem_ids[idx]);
		return 1;
	}
	int free = find_free_sem_slot();
	if (free < 0)
		return 0;
	uint64_t id = sys_sem_create((int) initialValue);
	if (!id)
		return 0;
	size_t len = strlen(sem_id) + 1;
	char *copy = (char *) malloc(len);
	if (!copy) {
		sys_sem_close(id);
		return 0;
	}
	for (size_t i = 0; i < len; ++i)
		copy[i] = sem_id[i];
	user_sem_names[free] = copy;
	user_sem_ids[free] = id;
	return 1;
}

int64_t my_sem_wait(char *sem_id) {
	if (!sem_id)
		return 0;
	int idx = find_sem_index(sem_id);
	if (idx < 0)
		return 0;
	return (int64_t) sys_sem_wait(user_sem_ids[idx]);
}

int64_t my_sem_post(char *sem_id) {
	if (!sem_id)
		return 0;
	int idx = find_sem_index(sem_id);
	if (idx < 0)
		return 0;
	return (int64_t) sys_sem_signal(user_sem_ids[idx]);
}

int64_t my_sem_close(char *sem_id) {
	if (!sem_id)
		return 0;
	int idx = find_sem_index(sem_id);
	if (idx < 0)
		return 0;
	uint64_t id = user_sem_ids[idx];
	int64_t res = (int64_t) sys_sem_close(id);
	free(user_sem_names[idx]);
	user_sem_names[idx] = NULL;
	user_sem_ids[idx] = 0;
	return res;
}
