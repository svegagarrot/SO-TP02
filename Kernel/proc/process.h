#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdint.h>

#define PROCESS_NAME_MAX_LEN 32
#define PROCESS_KERNEL_STACK_SIZE (16 * 1024)
#define PROCESS_USER_STACK_SIZE   (16 * 1024)

typedef enum {
    PROCESS_TYPE_KERNEL = 0,
    PROCESS_TYPE_USER   = 1
} process_type_t;

typedef enum {
    PROCESS_STATE_NEW = 0,
    PROCESS_STATE_READY,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_FINISHED
} process_state_t;

typedef enum {
    PROCESS_BACKGROUND = 0,
    PROCESS_FOREGROUND = 1
} process_mode_t;

struct process_control_block;

typedef void (*process_entry_point_t)(void *);
typedef struct process_control_block process_t;

typedef struct process_queue {
    process_t *head;
    process_t *tail;
    size_t size;
} process_queue_t;

struct process_control_block {
    uint64_t pid;
    char name[PROCESS_NAME_MAX_LEN + 1];
    int priority;
    process_state_t state;
    process_mode_t mode;
    process_type_t type;

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
process_t *process_create(const char *name,
                          process_entry_point_t entry_point,
                          void *entry_arg,
                          int priority,
                          process_mode_t mode,
                          process_type_t type,
                          process_t *parent);
void process_destroy(process_t *process);

void process_attach_child(process_t *parent, process_t *child);
void process_detach_child(process_t *child);

void process_queue_init(process_queue_t *queue);
void process_queue_push(process_queue_t *queue, process_t *process);
process_t *process_queue_pop(process_queue_t *queue);
void process_queue_remove(process_queue_t *queue, process_t *process);
int process_queue_is_empty(const process_queue_t *queue);

#endif
