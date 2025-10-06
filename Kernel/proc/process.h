#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stddef.h>

#define PROCESS_NAME_MAX_LEN 32
#define PROCESS_KERNEL_STACK_SIZE (4 * 4096)
#define PROCESS_USER_STACK_SIZE   (4 * 4096)

typedef struct process_control_block process_t;
typedef void (*process_entry_point_t)(void *);

typedef enum {
    PROCESS_STATE_NEW = 0,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_FINISHED
} process_state_t;

struct process_control_block {
    uint64_t pid;
    char     name[PROCESS_NAME_MAX_LEN + 1];

    process_state_t state;

    uint64_t rsp;
    uint64_t rbp;

    void *kernel_stack_base;
    void *kernel_stack_top;
    void *user_stack_base;
    void *user_stack_top;

    process_entry_point_t entry_point;
    void *entry_arg;

    process_t *parent;
    process_t *first_child;
    process_t *next_sibling;
    process_t *prev_sibling;

    process_t *queue_next;
    process_t *queue_prev;
};

void process_system_init(void);

process_t * process_create(const char *name,
                           process_entry_point_t entry_point,
                           void *entry_arg,
                           process_t *parent);

void process_destroy(process_t *p);

void process_attach_child(process_t *parent, process_t *child);
void process_detach_child(process_t *child);

typedef struct {
    process_t *head;
    process_t *tail;
    uint64_t   size;
} process_queue_t;

void process_queue_init(process_queue_t *q);
void process_queue_remove(process_queue_t *q, process_t *p);
void process_queue_push(process_queue_t *q, process_t *p);
process_t * process_queue_pop(process_queue_t *q);
int process_queue_is_empty(const process_queue_t *q);

#endif
