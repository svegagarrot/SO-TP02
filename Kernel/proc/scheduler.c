#include "scheduler.h"
#include "process.h"
#include "time.h"
#include "interrupts.h"

#define QUANTUM_TICKS 18

static process_queue_t ready_q;
static process_queue_t blocked_q;
static process_queue_t finished_q;

static process_t *current = NULL;
static process_t *idle_p  = NULL;
static uint64_t    last_switch_tick = 0;

static inline void sched_crit_enter(void) { _cli(); }
static inline void sched_crit_exit(void)  { _sti(); }

static void idle_entry(void *unused) {
    (void)unused;
    for (;;) { _hlt(); }
}

// Guardar contexto del proceso saliente y re-ubicarlo en la cola correcta
static void save_context(process_t *p, uint64_t current_rsp) {
    if (!p) {
        return;
    }

    p->rsp = current_rsp;

    // La pila en schedule() apunta al tope del pushState del handler:
    // [0]=r15, [1]=r14, [2]=r13, [3]=r12, [4]=r11, [5]=r10, [6]=r9, [7]=r8,
    // [8]=rsi, [9]=rdi, [10]=rbp, [11]=rdx, [12]=rcx, [13]=rbx, [14]=rax
    // Por lo tanto, para guardar el RBP correcto del proceso debemos leer índice 10.
    p->rbp = ((uint64_t *)current_rsp)[10];
    
    switch (p->state) {
        case PROCESS_STATE_RUNNING:
            if (p != idle_p) {
                p->state = PROCESS_STATE_READY; 
                // En IRQ context IF ya está deshabilitado, pero mantener la
                // convención de secciones críticas si se reutiliza.
                process_queue_push(&ready_q, p);
            } 
            break;
        case PROCESS_STATE_BLOCKED:
            process_queue_push(&blocked_q, p);
            break;
        case PROCESS_STATE_FINISHED:
            process_queue_push(&finished_q, p);
            break;
        default:
            break;
    }
}

void init_scheduler(void) {
    process_system_init();
    process_queue_init(&ready_q);
    process_queue_init(&blocked_q);
    process_queue_init(&finished_q);

    idle_p = process_create("idle", idle_entry, NULL, NULL);
    if (idle_p) {
        idle_p->state = PROCESS_STATE_RUNNING;
        current = idle_p;
        last_switch_tick = ticks_elapsed();
    } 
}

uint64_t schedule(uint64_t current_rsp) {
    if (!current) {
        return current_rsp;
    }

    uint64_t now = ticks_elapsed();

    if (process_queue_is_empty(&ready_q)) {
        if (current == idle_p) {
            last_switch_tick = now;
            return current_rsp;
        }
        if (current->state == PROCESS_STATE_RUNNING && (now - last_switch_tick) < QUANTUM_TICKS) {
            return current_rsp;
        }
    } 
    
    save_context(current, current_rsp);

    process_t *next = process_queue_pop(&ready_q);
    if (!next) {
        next = idle_p;
    } 

    next->state = PROCESS_STATE_RUNNING;
    current = next;
    last_switch_tick = now;

    // Recolectar y liberar procesos terminados para evitar fuga de memoria
    // que puede trabar el sistema después de muchos pids.
    for (process_t *fp = scheduler_collect_finished(); fp != NULL; fp = scheduler_collect_finished()) {
        process_destroy(fp);
    }
    return next->rsp;
}

process_t *scheduler_current_process(void) { 
    return current; 
}

void scheduler_add_process(process_t *p) {
    if (!p || p == idle_p) {
        return;
    }
    sched_crit_enter();
    p->state = PROCESS_STATE_READY;
    process_queue_push(&ready_q, p);
    sched_crit_exit();
}

process_t *scheduler_spawn_process(const char *name,
                                   process_entry_point_t entry_point,
                                   void *entry_arg,
                                   process_t *parent) {
    process_t *p = process_create(name, entry_point, entry_arg, parent);
    if (!p) {
        return NULL;
    }
    scheduler_add_process(p);
    return p;
}

void scheduler_block_current(void) {
    if (!current || current == idle_p) {
        return;
    }
    // Se marca bloqueado; el movimiento a cola sucede en save_context.
    current->state = PROCESS_STATE_BLOCKED;
}

void scheduler_unblock_process(process_t *p) {
    if (!p) {
        return;
    }
    sched_crit_enter();
    process_queue_remove(&blocked_q, p);
    p->state = PROCESS_STATE_READY;
    process_queue_push(&ready_q, p);
    sched_crit_exit();
}

void scheduler_finish_current(void) {
    if (!current || current == idle_p) {
        return;
    }
    // Se marca terminado; movimiento a cola en save_context.
    current->state = PROCESS_STATE_FINISHED;
}

process_t *scheduler_collect_finished(void) {
    sched_crit_enter();
    process_t *p = process_queue_pop(&finished_q);
    sched_crit_exit();
    return p;
}

static process_t *find_in_queue(process_queue_t *q, uint64_t pid) {
    for (process_t *it = q->head; it; it = it->queue_next) {
        if (it->pid == pid) {
            return it;
        }
    }
    return NULL;
}

process_t *scheduler_find_by_pid(uint64_t pid) {
    process_t *p;
    if ((p = find_in_queue(&ready_q, pid))) {
        return p;
    }
    if ((p = find_in_queue(&blocked_q, pid))) {
        return p;
    }
    if ((p = find_in_queue(&finished_q, pid))) {
        return p;
    }
    if (current && current->pid == pid) {
        return current;
    }
    return NULL;
}

int scheduler_unblock_by_pid(uint64_t pid) {
    process_t *p = scheduler_find_by_pid(pid);
    if (!p) {
        return 0;
    }
    scheduler_unblock_process(p);
    return 1;
}

int scheduler_kill_by_pid(uint64_t pid) {
    process_t *p = scheduler_find_by_pid(pid);
    if (!p) {
        return 0;
    }
    // marcar terminado y mover a finished
    sched_crit_enter();
    p->state = PROCESS_STATE_FINISHED;
    process_queue_remove(&ready_q, p);
    process_queue_remove(&blocked_q, p);
    process_queue_push(&finished_q, p);
    sched_crit_exit();
    return 1;
}
