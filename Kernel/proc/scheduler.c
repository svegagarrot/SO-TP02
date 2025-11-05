#include "scheduler.h"
#include "process.h"
#include "time.h"
#include "interrupts.h"
#include <naiveConsole.h>
#include <stddef.h>
#include <stdbool.h>

#define QUANTUM_TICKS 8
#define MAX_FINISHED_COLLECT 4

#define AGING_THRESHOLD 200     // Ticks de espera antes de boost (aumentado para debugging)
#define AGING_BOOST 1          // Cuánto aumentar la prioridad

// Flag global para forzar reschedule inmediato
static volatile int need_resched = 0;

// Round-robin con prioridades: 0..2 (baja..alta)
static process_queue_t ready_queues[PROCESS_PRIORITY_MAX - PROCESS_PRIORITY_MIN + 1];
static process_queue_t blocked_q;
static process_queue_t finished_q;

static process_t *current = NULL;
static process_t *idle_p  = NULL;
static uint64_t    last_switch_tick = 0;

static inline void sched_crit_enter(void) { _cli(); }
static inline void sched_crit_exit(void)  { _sti(); }

static inline uint8_t clamp_priority(uint8_t pr) {
    if (pr < PROCESS_PRIORITY_MIN) return PROCESS_PRIORITY_MIN;
    if (pr > PROCESS_PRIORITY_MAX) return PROCESS_PRIORITY_MAX;
    return pr;
}

static int ready_queues_are_empty(void) {
    for (int pr = PROCESS_PRIORITY_MIN; pr <= PROCESS_PRIORITY_MAX; ++pr) {
        if (!process_queue_is_empty(&ready_queues[pr])) return 0;
    }
    return 1;
}

static void ready_queue_push(process_t *p) {
    if (!p) return;
    p->priority = clamp_priority(p->priority);
    process_queue_push(&ready_queues[p->priority], p);
}

static process_t *ready_queue_pop_highest(void) {
    for (int pr = PROCESS_PRIORITY_MAX; pr >= PROCESS_PRIORITY_MIN; --pr) {
        process_t *p = process_queue_pop(&ready_queues[pr]);
        if (p) return p;
    }
    return NULL;
}

static void idle_entry(void *unused) {
    (void)unused;
    for (;;) { _hlt(); }
}

// Función de aging: incrementa wait_ticks y aplica boost si es necesario
static void apply_aging(void) {
    for (int pr = PROCESS_PRIORITY_MIN; pr <= PROCESS_PRIORITY_MAX; ++pr) {
        process_t *p = ready_queues[pr].head;
        while (p) {
            p->wait_ticks++;
            
            // Si esperó suficiente y no está en máxima prioridad, hacer boost
            if (p->wait_ticks >= AGING_THRESHOLD && p->priority < PROCESS_PRIORITY_MAX) {
                // Remover de la cola actual
                process_queue_remove(&ready_queues[pr], p);
                
                // Aumentar prioridad temporalmente
                p->priority = clamp_priority(p->priority + AGING_BOOST);
                
                // Reinsertar en la nueva cola de prioridad
                process_queue_push(&ready_queues[p->priority], p);
                
                // Resetear contador
                p->wait_ticks = 0;
                
                // Como modificamos la cola, reiniciar el recorrido
                p = ready_queues[pr].head;
            } else {
                p = p->queue_next;
            }
        }
    }
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
                ready_queue_push(p);
            } 
            break;
        case PROCESS_STATE_BLOCKED:
            process_queue_push(&blocked_q, p);
            break;
        case PROCESS_STATE_FINISHED:
            // Despertar procesos que esperan por este PID
            while (p->waiters_head) {
                process_t *w = p->waiters_head;
                p->waiters_head = w->waiter_next;
                w->waiter_next = NULL;
                w->waiting_on_pid = 0;
                w->state = PROCESS_STATE_READY;
                ready_queue_push(w);
            }
            process_queue_push(&finished_q, p);
            break;
        default:
            break;
    }
}

void init_scheduler(void) {
    process_system_init();
    for (int pr = PROCESS_PRIORITY_MIN; pr <= PROCESS_PRIORITY_MAX; ++pr) {
        process_queue_init(&ready_queues[pr]);
    }
    process_queue_init(&blocked_q);
    process_queue_init(&finished_q);

    idle_p = process_create("idle", idle_entry, NULL, NULL, 0);  // idle no es foreground
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

    // Aplicar aging para prevenir starvation
    apply_aging();
    apply_aging();  

    // Preemptivo: chequeo de need_resched y condiciones de cambio
    bool quantum_expired = (now - last_switch_tick) >= QUANTUM_TICKS;
    bool must_switch = (current->state != PROCESS_STATE_RUNNING);

    // ¿Hay uno de mayor prioridad listo?
    process_t *peek = NULL;
    for (int pr = PROCESS_PRIORITY_MAX; pr >= PROCESS_PRIORITY_MIN; --pr) {
        if (ready_queues[pr].head) {
            peek = ready_queues[pr].head;
            break;
        }
    }
    bool higher_pr_ready = peek && (!current || peek->priority > current->priority);

    must_switch = must_switch || higher_pr_ready || quantum_expired || need_resched;

    if (!must_switch && current->state == PROCESS_STATE_RUNNING) {
        return current_rsp;
    }

    need_resched = 0;
    save_context(current, current_rsp);

    process_t *next = ready_queue_pop_highest();
    if (!next) {
        next = idle_p;
    }

    next->state = PROCESS_STATE_RUNNING;
    
    // Resetear contador de espera cuando el proceso se ejecuta
    next->wait_ticks = 0;
    
    // Si la prioridad fue boosteada por aging, restaurar la base
    if (next != idle_p && next->priority > next->base_priority) {
        next->priority = next->base_priority;
    }
    
    current = next;
    last_switch_tick = now;

    // Limitar la cantidad de procesos terminados liberados por tick
    for (int i = 0; i < MAX_FINISHED_COLLECT; i++) {
        process_t *fp = scheduler_collect_finished();
        if (!fp) break;
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
    ready_queue_push(p);
    sched_crit_exit();
}

process_t *scheduler_spawn_process(const char *name,
                                   process_entry_point_t entry_point,
                                   void *entry_arg,
                                   process_t *parent,
                                   uint8_t priority,
                                   int is_foreground) {
    process_t *p = process_create(name, entry_point, entry_arg, parent, is_foreground);
    if (!p) {
        return NULL;
    }
    // Aplicar prioridad solicitada
    uint8_t pr = clamp_priority(priority);
    p->priority = pr;
    p->base_priority = pr;
    p->wait_ticks = 0;
    scheduler_add_process(p);
    return p;
}

void scheduler_block_current(void) {
    if (!current || current == idle_p) {
        return;
    }
    current->state = PROCESS_STATE_BLOCKED;
    need_resched = 1;
}

void scheduler_yield_current(void) {
    if (!current || current == idle_p) {
        return;
    }
    // Forzar expiración del quantum para cambio inmediato
    last_switch_tick = ticks_elapsed() - QUANTUM_TICKS;  // quantum = 0 (siguiente!!)
    callTimerTick();
}

void scheduler_unblock_process(process_t *p) {
    if (!p) {
        return;
    }
    sched_crit_enter();
    process_queue_remove(&blocked_q, p);
    p->state = PROCESS_STATE_READY;
    ready_queue_push(p);
    // Si tiene mayor prioridad que el actual, forzar reschedule
    if (current && p->priority > current->priority)
        need_resched = 1;
    sched_crit_exit();
}

void scheduler_finish_current(void) {
    if (!current || current == idle_p) {
        return;
    }
    current->state = PROCESS_STATE_FINISHED;
    need_resched = 1;
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
    for (int pr = PROCESS_PRIORITY_MIN; pr <= PROCESS_PRIORITY_MAX; ++pr) {
        if ((p = find_in_queue(&ready_queues[pr], pid))) {
            return p;
        }
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
    for (int pr = PROCESS_PRIORITY_MIN; pr <= PROCESS_PRIORITY_MAX; ++pr) {
        process_queue_remove(&ready_queues[pr], p);
    }
    process_queue_remove(&blocked_q, p);
    // Despertar waiters si los hay
    while (p->waiters_head) {
        process_t *w = p->waiters_head;
        p->waiters_head = w->waiter_next;
        w->waiter_next = NULL;
        w->waiting_on_pid = 0;
        w->state = PROCESS_STATE_READY;
        ready_queue_push(w);
    }
    process_queue_push(&finished_q, p);
    sched_crit_exit();
    if (p == current)
        need_resched = 1;
    return 1;
}

int scheduler_set_priority(uint64_t pid, uint8_t new_priority) {
    process_t *p = scheduler_find_by_pid(pid);
    if (!p || p == idle_p) return 0;

    sched_crit_enter();
    uint8_t clamped = clamp_priority(new_priority);
    int oldp = p->priority;
    if (oldp == clamped) {
        sched_crit_exit();
        return 1;
    }

    if (p->state == PROCESS_STATE_READY) {
        // Sacar de su cola actual
        process_queue_remove(&ready_queues[oldp], p);
    }

    p->priority = clamped;
    p->base_priority = clamped;  // También actualizar base_priority
    p->wait_ticks = 0;  // Resetear aging counter cuando se cambia prioridad manualmente

    if (p->state == PROCESS_STATE_READY) {
        ready_queue_push(p);
    }
    // Si sube sobre el actual, forzar resched
    if (current && p->priority > current->priority)
        need_resched = 1;
    sched_crit_exit();
    return 1;
}

int scheduler_block_by_pid(uint64_t pid) {
    process_t *p = scheduler_find_by_pid(pid);
    if (!p || p == idle_p) return 0;

    sched_crit_enter();
    if (p == current) {
        // Marcar y que el movimiento ocurra en save_context
        current->state = PROCESS_STATE_BLOCKED;
        need_resched = 1;
        sched_crit_exit();
        return 1;
    }

    if (p->state == PROCESS_STATE_READY) {
        process_queue_remove(&ready_queues[p->priority], p);
        p->state = PROCESS_STATE_BLOCKED;
        process_queue_push(&blocked_q, p);
        need_resched = 1;
        sched_crit_exit();
        return 1;
    }
    // Si ya está bloqueado o terminado no hacemos nada
    sched_crit_exit();
    return 1;
}

static void add_process_to_list(process_t *p, process_info_t *buffer, uint64_t *count, uint64_t max_count) {
    if (!p || !buffer || *count >= max_count || p == idle_p) {
        return;
    }

    buffer[*count].pid = p->pid;
    
    // Copiar nombre asegurando terminador nulo
    int i;
    for (i = 0; i < PROCESS_NAME_MAX_LEN && p->name[i] != '\0'; i++) {
        buffer[*count].name[i] = p->name[i];
    }
    buffer[*count].name[i] = '\0';  // Asegurar terminador en la posición correcta
    
    buffer[*count].state = p->state;
    buffer[*count].priority = p->priority;
    buffer[*count].rsp = p->rsp;
    buffer[*count].rbp = p->rbp;
    buffer[*count].foreground = p->is_foreground;  // Usar is_foreground del proceso
    (*count)++;
}

uint64_t scheduler_list_all_processes(process_info_t *buffer, uint64_t max_count) {
    if (!buffer || max_count == 0) {
        return 0;
    }

    sched_crit_enter();
    uint64_t count = 0;

    // Agregar procesos de las colas ready por prioridad
    for (int pr = PROCESS_PRIORITY_MIN; pr <= PROCESS_PRIORITY_MAX; ++pr) {
        for (process_t *p = ready_queues[pr].head; p && count < max_count; p = p->queue_next) {
            add_process_to_list(p, buffer, &count, max_count);
        }
    }

    // Agregar procesos bloqueados
    for (process_t *p = blocked_q.head; p && count < max_count; p = p->queue_next) {
        add_process_to_list(p, buffer, &count, max_count);
    }

    // Agregar procesos terminados
    for (process_t *p = finished_q.head; p && count < max_count; p = p->queue_next) {
        add_process_to_list(p, buffer, &count, max_count);
    }

    // Agregar proceso actual (si no está ya en alguna cola)
    if (current && count < max_count) {
        // Verificar si current ya fue agregado
        int already_added = 0;
        for (uint64_t i = 0; i < count; i++) {
            if (buffer[i].pid == current->pid) {
                already_added = 1;
                // Actualizar la información del proceso current para asegurar que tiene foreground=1
                buffer[i].state = current->state;
                buffer[i].rsp = current->rsp;
                buffer[i].rbp = current->rbp;
                buffer[i].foreground = current->is_foreground;  // Usar is_foreground del proceso
                break;
            }
        }
        if (!already_added) {
            add_process_to_list(current, buffer, &count, max_count);
        }
    }

    sched_crit_exit();
    return count;
}
