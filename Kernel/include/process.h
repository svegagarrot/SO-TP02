#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdint.h>

#define PROCESS_NAME_MAX_LEN 32
#define PROCESS_KERNEL_STACK_SIZE (4 * 4096)
#define PROCESS_USER_STACK_SIZE (4 * 4096)

#define PROCESS_PRIORITY_MIN 0
#define PROCESS_PRIORITY_MAX 2
#define PROCESS_PRIORITY_DEFAULT 1

#define MAX_FDS 16 // Número máximo de file descriptors por proceso

typedef struct process_control_block process_t;
typedef void (*process_entry_point_t)(void *);

typedef struct pipe_t pipe_t;

typedef enum { FD_TYPE_TERMINAL = 0, FD_TYPE_PIPE_READ = 1, FD_TYPE_PIPE_WRITE = 2 } fd_type_t;

typedef struct {
	fd_type_t type;
	pipe_t *pipe;
} fd_entry_t;

typedef enum {
	PROCESS_STATE_NEW = 0,
	PROCESS_STATE_READY,
	PROCESS_STATE_RUNNING,
	PROCESS_STATE_BLOCKED,
	PROCESS_STATE_FINISHED
} process_state_t;

struct process_control_block {
	uint64_t pid;
	char name[PROCESS_NAME_MAX_LEN + 1];

	process_state_t state;
	int priority;
	int base_priority;
	uint64_t wait_ticks;

	uint64_t rsp;
	uint64_t rbp;

	void *kernel_stack_base;
	void *kernel_stack_top;
	void *user_stack_base;
	void *user_stack_top;

	process_entry_point_t entry_point;
	void *entry_arg;
	uint64_t waiting_on_pid;
	int is_foreground;
	process_t *parent;
	process_t *first_child;
	process_t *next_sibling;
	process_t *prev_sibling;

	process_t *queue_next;
	process_t *queue_prev;
	process_t *waiters_head;
	process_t *waiter_next;

	uint64_t waiting_on_sem;

	process_t *sem_waiter_next;


	fd_entry_t fds[MAX_FDS];
};

void process_system_init(void);

process_t *process_create(const char *name, process_entry_point_t entry_point, void *entry_arg, process_t *parent,
						  int is_foreground, uint64_t stdin_pipe_id, uint64_t stdout_pipe_id);

void process_destroy(process_t *p);

void process_close_fds(process_t *p);

void process_attach_child(process_t *parent, process_t *child);
void process_detach_child(process_t *child);

typedef struct {
	process_t *head;
	process_t *tail;
	uint64_t size;
} process_queue_t;

void process_queue_init(process_queue_t *q);
void process_queue_remove(process_queue_t *q, process_t *p);
void process_queue_push(process_queue_t *q, process_t *p);
process_t *process_queue_pop(process_queue_t *q);
int process_queue_is_empty(const process_queue_t *q);

#define MAX_PROCESS_INFO 64
typedef struct {
	uint64_t pid;
	char name[PROCESS_NAME_MAX_LEN + 1];
	process_state_t state;
	int priority;
	uint64_t rsp;
	uint64_t rbp;
	int foreground;
} process_info_t;

#endif
