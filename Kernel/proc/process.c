#include "process.h"
#include "mm.h"
#include "interrupts.h"
#include "pipe.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1

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
                          int is_foreground,
                          uint64_t stdin_pipe_id,
                          uint64_t stdout_pipe_id) {
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

    // Inicializar tabla de file descriptors
    // STDIN (fd 0) y STDOUT (fd 1) apuntan a terminal por defecto
    for (int i = 0; i < MAX_FDS; i++) {
        p->fds[i].type = FD_TYPE_TERMINAL;
        p->fds[i].pipe = NULL;
    }
    
    // Configurar stdin si se especificó un pipe
    if (stdin_pipe_id != 0) {
        pipe_t *stdin_pipe = pipe_get_by_id(stdin_pipe_id);
        if (stdin_pipe && pipe_open_by_id(stdin_pipe_id, 0)) {  // is_writer=0 (lector)
            p->fds[STDIN].type = FD_TYPE_PIPE_READ;
            p->fds[STDIN].pipe = stdin_pipe;
        }
    }
    
    // Configurar stdout si se especificó un pipe
    if (stdout_pipe_id != 0) {
        pipe_t *stdout_pipe = pipe_get_by_id(stdout_pipe_id);
        if (stdout_pipe && pipe_open_by_id(stdout_pipe_id, 1)) {  // is_writer=1 (escritor)
            p->fds[STDOUT].type = FD_TYPE_PIPE_WRITE;
            p->fds[STDOUT].pipe = stdout_pipe;
        }
    }
    
    // Heredar otros file descriptors del padre (excepto stdin/stdout si fueron redirigidos)
    if (parent) {
        for (int i = 0; i < MAX_FDS; ++i) {
            // Skip stdin/stdout si fueron redirigidos
            if ((i == STDIN && stdin_pipe_id != 0) || (i == STDOUT && stdout_pipe_id != 0)) {
                continue;
            }
            
            fd_entry_t *parent_fd = &parent->fds[i];
            fd_entry_t *child_fd = &p->fds[i];

            child_fd->type = parent_fd->type;

            if (parent_fd->type == FD_TYPE_PIPE_READ || parent_fd->type == FD_TYPE_PIPE_WRITE) {
                if (parent_fd->pipe) {
                    uint64_t pipe_id = pipe_get_id(parent_fd->pipe);
                    if (pipe_id != 0) {
                        // Determinar is_writer según el tipo de FD del padre
                        int is_writer = (parent_fd->type == FD_TYPE_PIPE_WRITE) ? 1 : 0;
                        if (pipe_open_by_id(pipe_id, is_writer)) {
                            child_fd->pipe = parent_fd->pipe;
                        } else {
                            child_fd->type = FD_TYPE_TERMINAL;
                            child_fd->pipe = NULL;
                        }
                    } else {
                        child_fd->type = FD_TYPE_TERMINAL;
                        child_fd->pipe = NULL;
                    }
                } else {
                    child_fd->type = FD_TYPE_TERMINAL;
                    child_fd->pipe = NULL;
                }
            } else {
                child_fd->pipe = NULL;
            }
        }
    }

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

    // Cerrar todos los file descriptors que sean pipes
    for (int i = 0; i < MAX_FDS; i++) {
        if (p->fds[i].type == FD_TYPE_PIPE_READ || p->fds[i].type == FD_TYPE_PIPE_WRITE) {
            if (p->fds[i].pipe) {
                // Obtener el ID del pipe para cerrarlo
                uint64_t pipe_id = pipe_get_id(p->fds[i].pipe);
                if (pipe_id != 0) {
                    // Determinar si es escritor o lector basado en el tipo de FD
                    int is_writer = (p->fds[i].type == FD_TYPE_PIPE_WRITE) ? 1 : 0;
                    pipe_close_by_id(pipe_id, is_writer);
                }
            }
            // Limpiar la entrada
            p->fds[i].type = FD_TYPE_TERMINAL;
            p->fds[i].pipe = NULL;
        }
    }

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

// Cerrar los file descriptors de un proceso sin destruirlo
void process_close_fds(process_t *p) {
    if (!p) return;

    for (int i = 0; i < MAX_FDS; i++) {
        if (p->fds[i].type == FD_TYPE_PIPE_READ || p->fds[i].type == FD_TYPE_PIPE_WRITE) {
            if (p->fds[i].pipe) {
                uint64_t pipe_id = pipe_get_id(p->fds[i].pipe);
                if (pipe_id != 0) {
                    // Determinar si es escritor o lector basado en el tipo de FD
                    int is_writer = (p->fds[i].type == FD_TYPE_PIPE_WRITE) ? 1 : 0;
                    pipe_close_by_id(pipe_id, is_writer);
                }
            }
            // Limpiar INMEDIATAMENTE para evitar cierre doble
            p->fds[i].type = FD_TYPE_TERMINAL;
            p->fds[i].pipe = NULL;
        }
    }
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
