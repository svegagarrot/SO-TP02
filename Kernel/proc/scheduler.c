#include "scheduler.h"
#include "time.h"
#include "interrupts.h"

#define QUANTUM_TICKS 18

static process_queue_t ready_queue;
static process_queue_t blocked_queue;
static process_queue_t finished_queue;

static process_t *current_process = NULL;
static process_t *idle_process_pcb = NULL;
static uint64_t last_switch_tick = 0;

static void idle_process_entry(void *unused) {
    (void)unused;
    while (1) {
        _hlt();
    }
}

static void save_context(process_t *process, uint64_t current_rsp) {
    if (process == NULL) {
        return;
    }

    process->rsp = current_rsp;
    process->rbp = ((uint64_t *)current_rsp)[10];

    switch (process->state) {
        case PROCESS_STATE_RUNNING:
            if (process != idle_process_pcb) {
                process->state = PROCESS_STATE_READY;
                process_queue_push(&ready_queue, process);
            }
            break;
        case PROCESS_STATE_BLOCKED:
            process_queue_push(&blocked_queue, process);
            break;
        case PROCESS_STATE_FINISHED:
            process_queue_push(&finished_queue, process);
            break;
        default:
            break;
    }
}

void init_scheduler(void) {
    process_system_init();
    process_queue_init(&ready_queue);
    process_queue_init(&blocked_queue);
    process_queue_init(&finished_queue);

    idle_process_pcb = process_create("idle",
                                      idle_process_entry,
                                      NULL,
                                      -1,
                                      PROCESS_BACKGROUND,
                                      PROCESS_TYPE_KERNEL,
                                      NULL);

    if (idle_process_pcb != NULL) {
        idle_process_pcb->state = PROCESS_STATE_RUNNING;
        current_process = idle_process_pcb;
        last_switch_tick = ticks_elapsed();
    }
}

uint64_t schedule(uint64_t current_rsp) {
    if (current_process == NULL) {
        return current_rsp;
    }

    uint64_t now = ticks_elapsed();

    if (current_process == idle_process_pcb && process_queue_is_empty(&ready_queue)) {
        last_switch_tick = now;
        return current_rsp;
    }

    if (current_process->state == PROCESS_STATE_RUNNING && process_queue_is_empty(&ready_queue)) {
        if ((now - last_switch_tick) < QUANTUM_TICKS) {
            return current_rsp;
        }
    }

    save_context(current_process, current_rsp);

    process_t *next = process_queue_pop(&ready_queue);
    if (next == NULL) {
        next = idle_process_pcb;
    }

    next->state = PROCESS_STATE_RUNNING;
    current_process = next;
    last_switch_tick = now;

    return next->rsp;
}

process_t *scheduler_current_process(void) {
    return current_process;
}

void scheduler_add_process(process_t *process) {
    if (process == NULL || process == idle_process_pcb) {
        return;
    }

    process->state = PROCESS_STATE_READY;
    process_queue_push(&ready_queue, process);
}

process_t *scheduler_spawn_process(const char *name,
                                   process_entry_point_t entry_point,
                                   void *entry_arg,
                                   int priority,
                                   process_mode_t mode,
                                   process_type_t type,
                                   process_t *parent) {
    process_t *process = process_create(name, entry_point, entry_arg, priority, mode, type, parent);
    if (process == NULL) {
        return NULL;
    }
    scheduler_add_process(process);
    return process;
}

void scheduler_block_current(void) {
    if (current_process == NULL || current_process == idle_process_pcb) {
        return;
    }

    current_process->state = PROCESS_STATE_BLOCKED;
}

void scheduler_unblock_process(process_t *process) {
    if (process == NULL) {
        return;
    }

    process_queue_remove(&blocked_queue, process);
    process->state = PROCESS_STATE_READY;
    process_queue_push(&ready_queue, process);
}

void scheduler_finish_current(void) {
    if (current_process == NULL || current_process == idle_process_pcb) {
        return;
    }

    current_process->state = PROCESS_STATE_FINISHED;
}

process_t *scheduler_collect_finished(void) {
    return process_queue_pop(&finished_queue);
}

process_t *scheduler_find_by_pid(uint64_t pid) {
    // Buscar en ready queue
    process_t *current = ready_queue.head;
    while (current != NULL) {
        if (current->pid == pid) {
            return current;
        }
        current = current->queue_next;
    }
    
    // Buscar en blocked queue
    current = blocked_queue.head;
    while (current != NULL) {
        if (current->pid == pid) {
            return current;
        }
        current = current->queue_next;
    }
    
    // Buscar en finished queue
    current = finished_queue.head;
    while (current != NULL) {
        if (current->pid == pid) {
            return current;
        }
        current = current->queue_next;
    }
    
    // Buscar en current process
    if (current_process != NULL && current_process->pid == pid) {
        return current_process;
    }
    
    return NULL;
}

int scheduler_unblock_by_pid(uint64_t pid) {
    process_t *process = scheduler_find_by_pid(pid);
    if (process == NULL) {
        return 0;
    }
    scheduler_unblock_process(process);
    return 1;
}

int scheduler_kill_by_pid(uint64_t pid) {
    process_t *process = scheduler_find_by_pid(pid);
    if (process == NULL) {
        return 0;
    }
    
    // Marcar como terminado y dejar que el scheduler lo limpie
    process->state = PROCESS_STATE_FINISHED;
    process_queue_remove(&ready_queue, process);
    process_queue_remove(&blocked_queue, process);
    process_queue_push(&finished_queue, process);
    
    return 1;
}

