#include "process.h"
#include "mm.h"
#include "interrupts.h"
#include "lib.h"
#include <stddef.h>
#include <stdint.h>

#define KERNEL_CODE_SELECTOR 0x08
#define INITIAL_RFLAGS 0x202

static uint64_t next_pid = 1;

static void copy_process_name(process_t *process, const char *name) {
    const char *source = (name != NULL) ? name : "proc";
    size_t i = 0;
    for (; i < PROCESS_NAME_MAX_LEN && source[i] != '\0'; i++) {
        process->name[i] = source[i];
    }
    if (i <= PROCESS_NAME_MAX_LEN) {
        process->name[i] = '\0';
    } else {
        process->name[PROCESS_NAME_MAX_LEN] = '\0';
    }
}

//reservo memoria para el stack del proceso
static void *allocate_stack(uint64_t size) {
    return mm_alloc(size);
}

static void release_stack(void *base) {
    if (base != NULL) {
        mm_free(base);
    }
}

static inline void zero_register_frame(uint64_t *frame_start) {
    for (size_t i = 0; i < 15; i++) {
        frame_start[i] = 0;
    }
}

static void process_launch(process_t *process) __attribute__((noreturn));
extern void scheduler_finish_current(void);

extern void process_user_entry(void *stack_top,
                               process_entry_point_t entry_point,
                               void *arg);

static void process_exit_trap(void) __attribute__((noreturn));

static void process_exit_trap(void) {
    scheduler_finish_current();
    while (1) {
        _hlt();
    }
}
//arranca el proceso
static void process_launch(process_t *process) {
    if (process == NULL) {
        process_exit_trap();
    }

    if (process->entry_point == NULL) {
        process_exit_trap();
    }

    if (process->type == PROCESS_TYPE_USER && process->user_stack_top != NULL) {
        process_user_entry(process->user_stack_top, process->entry_point, process->entry_arg);
    } else {
        process->entry_point(process->entry_arg);
    }

    process_exit_trap();
}

//prepara el contexto inicial del proceso
static void setup_initial_context(process_t *process) {
    uint8_t *stack_memory = (uint8_t *)process->kernel_stack_base;
    uint64_t *stack_top = (uint64_t *)(stack_memory + PROCESS_KERNEL_STACK_SIZE);
    uint64_t *stack = stack_top;

    const uint64_t rip = (uint64_t)process_launch;

    *--stack = rip;                   // RIP
    *--stack = KERNEL_CODE_SELECTOR;  // CS
    *--stack = INITIAL_RFLAGS;        // RFLAGS

    uint64_t registers[15];
    zero_register_frame(registers);

    registers[4] = (uint64_t)process->kernel_stack_top; // RBP
    registers[5] = (uint64_t)process;                   // RDI -> process pointer

    for (size_t i = 0; i < 15; i++) {
        *--stack = registers[i];
    }

    process->rsp = (uint64_t)stack;
    process->rbp = (uint64_t)process->kernel_stack_top;
}

void process_system_init(void) {
    next_pid = 1;
}

process_t *process_create(const char *name,
                          process_entry_point_t entry_point,
                          void *entry_arg,
                          int priority,
                          process_mode_t mode,
                          process_type_t type,
                          process_t *parent) {
    if (entry_point == NULL) {
        return NULL;
    }

    process_t *process = (process_t *)mm_alloc(sizeof(process_t));
    if (process == NULL) {
        return NULL;
    }

    memset(process, 0, sizeof(process_t));

    //reserva memoria para el stack del kernel y del usuario (si es necesario)
    process->kernel_stack_base = allocate_stack(PROCESS_KERNEL_STACK_SIZE);
    if (process->kernel_stack_base == NULL) {
        mm_free(process);
        return NULL;
    }

    process->kernel_stack_top = (uint8_t *)process->kernel_stack_base + PROCESS_KERNEL_STACK_SIZE;

    if (type == PROCESS_TYPE_USER) {
        process->user_stack_base = allocate_stack(PROCESS_USER_STACK_SIZE);
        if (process->user_stack_base == NULL) {
            release_stack(process->kernel_stack_base);
            mm_free(process);
            return NULL;
        }
        process->user_stack_top = (uint8_t *)process->user_stack_base + PROCESS_USER_STACK_SIZE;
    }

    process->pid = next_pid++;
    process->priority = priority;
    process->mode = mode;
    process->type = type;
    process->entry_point = entry_point;
    process->entry_arg = entry_arg;
    process->state = PROCESS_STATE_NEW;

    copy_process_name(process, name);

    process->parent = NULL;
    process->first_child = NULL;
    process->next_sibling = NULL;
    process->prev_sibling = NULL;
    process->queue_next = NULL;
    process->queue_prev = NULL;

    if (parent != NULL) {
        process_attach_child(parent, process);
    }

    setup_initial_context(process);
    process->state = PROCESS_STATE_READY;

    return process;
}

void process_destroy(process_t *process) {
    if (process == NULL) {
        return;
    }

    if (process->parent != NULL) {
        process_detach_child(process);
    }

    process_t *child = process->first_child;
    while (child != NULL) {
        process_t *next_child = child->next_sibling;
        child->parent = NULL;
        child->prev_sibling = NULL;
        child->next_sibling = NULL;
        child = next_child;
    }

    release_stack(process->kernel_stack_base);
    release_stack(process->user_stack_base);
    mm_free(process);
}

void process_attach_child(process_t *parent, process_t *child) {
    if (parent == NULL || child == NULL) {
        return;
    }

    child->parent = parent;
    child->next_sibling = parent->first_child;
    child->prev_sibling = NULL;
    if (parent->first_child != NULL) {
        parent->first_child->prev_sibling = child;
    }
    parent->first_child = child;
}

void process_detach_child(process_t *child) {
    if (child == NULL || child->parent == NULL) {
        return;
    }

    process_t *parent = child->parent;

    if (child->prev_sibling != NULL) {
        child->prev_sibling->next_sibling = child->next_sibling;
    } else if (parent->first_child == child) {
        parent->first_child = child->next_sibling;
    }

    if (child->next_sibling != NULL) {
        child->next_sibling->prev_sibling = child->prev_sibling;
    }

    child->parent = NULL;
    child->next_sibling = NULL;
    child->prev_sibling = NULL;
}

void process_queue_init(process_queue_t *queue) {
    if (queue == NULL) {
        return;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
}

static void detach_from_queue(process_queue_t *queue, process_t *process) {
    if (queue == NULL || process == NULL) {
        return;
    }

    int in_queue = (process->queue_next != NULL) || (process->queue_prev != NULL) || (queue->head == process);
    if (!in_queue) {
        return;
    }

    if (process->queue_prev != NULL) {
        process->queue_prev->queue_next = process->queue_next;
    } else {
        queue->head = process->queue_next;
    }

    if (process->queue_next != NULL) {
        process->queue_next->queue_prev = process->queue_prev;
    } else {
        queue->tail = process->queue_prev;
    }

    if (queue->size > 0) {
        queue->size--;
    }

    process->queue_next = NULL;
    process->queue_prev = NULL;
}

void process_queue_remove(process_queue_t *queue, process_t *process) {
    detach_from_queue(queue, process);
}

void process_queue_push(process_queue_t *queue, process_t *process) {
    if (queue == NULL || process == NULL) {
        return;
    }

    process_queue_remove(queue, process);

    if (queue->head == NULL) {
        queue->head = process;
        queue->tail = process;
        process->queue_next = NULL;
        process->queue_prev = NULL;
        queue->size = 1;
        return;
    }

    process_t *current = queue->head;
    while (current != NULL && current->priority >= process->priority) {
        current = current->queue_next;
    }

    if (current == NULL) {
        process->queue_prev = queue->tail;
        process->queue_next = NULL;
        queue->tail->queue_next = process;
        queue->tail = process;
    } else if (current->queue_prev == NULL) {
        process->queue_next = current;
        process->queue_prev = NULL;
        current->queue_prev = process;
        queue->head = process;
    } else {
        process->queue_next = current;
        process->queue_prev = current->queue_prev;
        current->queue_prev->queue_next = process;
        current->queue_prev = process;
    }

    queue->size++;
}

process_t *process_queue_pop(process_queue_t *queue) {
    if (queue == NULL || queue->head == NULL) {
        return NULL;
    }

    process_t *first = queue->head;
    detach_from_queue(queue, first);
    return first;
}

int process_queue_is_empty(const process_queue_t *queue) {
    return queue == NULL || queue->head == NULL;
}
