#include "process.h"
#include "mm.h"
#include "interrupts.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>


extern uint64_t setup_process_context(void *stack_top, void *entry_point, void *arg);

static uint64_t next_pid = 1;

static void copy_name(process_t *p, const char *name) {
    const char *src = (name ? name : "proc");
    size_t i = 0;
    for (; i < PROCESS_NAME_MAX_LEN && src[i]; ++i) p->name[i] = src[i];
    p->name[i] = '\0';
}

void process_system_init(void) { next_pid = 1; }

process_t *process_create(const char *name,
                          process_entry_point_t entry_point,
                          void *entry_arg,
                          process_t *parent,
                          int is_foreground) {
    if (!entry_point) return NULL;

    process_t *p = (process_t *)mm_alloc(sizeof(process_t));
    if (!p) return NULL;
    memset(p, 0, sizeof(*p));

    p->kernel_stack_base = mm_alloc(PROCESS_KERNEL_STACK_SIZE);
    if (!p->kernel_stack_base) {
        mm_free(p);
        return NULL;
    }
    p->kernel_stack_top = (uint8_t *)p->kernel_stack_base + PROCESS_KERNEL_STACK_SIZE;

    p->user_stack_base = NULL;
    p->user_stack_top  = NULL;

    p->pid         = next_pid++;
    p->entry_point = entry_point;
    p->entry_arg   = entry_arg;
    p->state       = PROCESS_STATE_NEW;
    p->is_foreground = is_foreground;  // Inicializar campo is_foreground
    p->priority      = PROCESS_PRIORITY_DEFAULT;
    p->base_priority = PROCESS_PRIORITY_DEFAULT;
    p->wait_ticks    = 0;

    copy_name(p, name);

    p->parent       = NULL;
    p->first_child  = NULL;
    p->next_sibling = NULL;
    p->prev_sibling = NULL;

    p->queue_next = NULL;
    p->queue_prev = NULL;

    if (parent) {
        p->parent       = parent;
        p->next_sibling = parent->first_child;
        p->prev_sibling = NULL;
        if (parent->first_child) parent->first_child->prev_sibling = p;
        parent->first_child = p;
    }

    p->rsp = setup_process_context(p->kernel_stack_top,
                                   (void *)p->entry_point,
                                   (void *)p->entry_arg);
    p->rbp = (uint64_t)p->kernel_stack_top;

    p->state = PROCESS_STATE_READY;
    return p;
}

void process_destroy(process_t *p) {
    if (!p) return;

    if (p->parent) {
        if (p->prev_sibling) p->prev_sibling->next_sibling = p->next_sibling;
        else if (p->parent->first_child == p) p->parent->first_child = p->next_sibling;
        if (p->next_sibling) p->next_sibling->prev_sibling = p->prev_sibling;
        p->parent = p->next_sibling = p->prev_sibling = NULL;
    }

    if (p->kernel_stack_base) mm_free(p->kernel_stack_base);
    if (p->user_stack_base)   mm_free(p->user_stack_base);
    mm_free(p);
}

void process_queue_init(process_queue_t *q) {
    if (!q) return;
    q->head = q->tail = NULL;
    q->size = 0;
}

static void detach_from_queue(process_queue_t *q, process_t *p) {
    if (!q || !p) return;

    // Verificar pertenencia: recorrer la cola y confirmar que 'p' está en 'q'.
    process_t *it = q->head;
    int found = 0;
    while (it) {
        if (it == p) { found = 1; break; }
        it = it->queue_next;
    }
    if (!found) return; // 'p' no pertenece a esta cola; no hacer nada.

    if (p->queue_prev) {
        p->queue_prev->queue_next = p->queue_next;
    } else {
        q->head = p->queue_next;
    }

    if (p->queue_next) {
        p->queue_next->queue_prev = p->queue_prev;
    } else {
        q->tail = p->queue_prev;
    }

    if (q->size) q->size--;
    p->queue_next = NULL;
    p->queue_prev = NULL;
}

void process_queue_remove(process_queue_t *q, process_t *p) { detach_from_queue(q, p); }

void process_queue_push(process_queue_t *q, process_t *p) {
    if (!q || !p) return;
    detach_from_queue(q, p);
    if (!q->head) {
        q->head = q->tail = p;
        p->queue_next = p->queue_prev = NULL;
        q->size = 1;
        return;
    }
    p->queue_prev = q->tail;
    p->queue_next = NULL;
    q->tail->queue_next = p;
    q->tail = p;
    q->size++;
}

process_t *process_queue_pop(process_queue_t *q) {
    if (!q || !q->head) return NULL;
    process_t *first = q->head;
    detach_from_queue(q, first);
    return first;
}

int process_queue_is_empty(const process_queue_t *q) {
    return (!q || q->head == NULL);
}
