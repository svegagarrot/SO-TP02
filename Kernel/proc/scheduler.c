#include "scheduler.h"
#include "process.h"
#include "time.h"
#include "interrupts.h"
#include "keyboardDriver.h"
#include <naiveConsole.h>
#include <stddef.h>
#include <stdbool.h>

#define QUANTUM_TICKS 8
#define MAX_FINISHED_COLLECT 4

#define AGING_THRESHOLD 200
#define AGING_BOOST 1

static volatile int need_resched = 0;

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

static void apply_aging(void) {
    for (int pr = PROCESS_PRIORITY_MIN; pr <= PROCESS_PRIORITY_MAX; ++pr) {
        process_t *p = ready_queues[pr].head;
        while (p) {
            p->wait_ticks++;
            
            if (p->wait_ticks >= AGING_THRESHOLD && p->priority < PROCESS_PRIORITY_MAX) {
                process_queue_remove(&ready_queues[pr], p);
                p->priority = clamp_priority(p->priority + AGING_BOOST);
                process_queue_push(&ready_queues[p->priority], p);
                p->wait_ticks = 0;
                p = ready_queues[pr].head;
            } else {
                p = p->queue_next;
            }
        }
    }
}

static void save_context(process_t *p, uint64_t current_rsp) {
    if (!p) {
        return;
    }

    p->rsp = current_rsp;
    p->rbp = ((uint64_t *)current_rsp)[10];
    
    switch (p->state) {
        case PROCESS_STATE_RUNNING:
            if (p != idle_p) {
                p->state = PROCESS_STATE_READY; 
                ready_queue_push(p);
            } 
            break;
        case PROCESS_STATE_BLOCKED:
            process_queue_push(&blocked_q, p);
            break;
        case PROCESS_STATE_FINISHED:
            if (p->is_foreground && p->parent && p->parent->state != PROCESS_STATE_FINISHED) {
                p->parent->is_foreground = 1;
                keyboard_clear_buffer();
            }
            
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

    idle_p = process_create("idle", idle_entry, NULL, NULL, 0, 0, 0);  // idle no es foreground, sin pipes
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

    apply_aging();

    bool quantum_expired = (now - last_switch_tick) >= QUANTUM_TICKS;
    bool must_switch = (current->state != PROCESS_STATE_RUNNING);

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
    next->wait_ticks = 0;
    
    if (next != idle_p && next->priority > next->base_priority) {
        next->priority = next->base_priority;
    }
    
    current = next;
    last_switch_tick = now;

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
                                   int is_foreground,
                                   uint64_t stdin_pipe_id,
                                   uint64_t stdout_pipe_id) {
    process_t *p = process_create(name, entry_point, entry_arg, parent, is_foreground, stdin_pipe_id, stdout_pipe_id);
    if (!p) {
        return NULL;
    }
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
    last_switch_tick = ticks_elapsed() - QUANTUM_TICKS;
    callTimerTick();
}

void scheduler_unblock_process(process_t *p) {
    if (!p) {
        return;
    }
    
    sched_crit_enter();
    
    // Solo remover de blocked_q si está BLOCKED
    if (p->state == PROCESS_STATE_BLOCKED) {
        process_queue_remove(&blocked_q, p);
    }
    
    p->state = PROCESS_STATE_READY;
    ready_queue_push(p);
    
    if (current && p->priority > current->priority)
        need_resched = 1;
    
    sched_crit_exit();
}

void scheduler_finish_current(void) {
    if (!current || current == idle_p) {
        return;
    }
    
    process_close_fds(current);
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
    
    // Buscar en ready queues
    for (int pr = PROCESS_PRIORITY_MIN; pr <= PROCESS_PRIORITY_MAX; ++pr) {
        if ((p = find_in_queue(&ready_queues[pr], pid))) {
            return p;
        }
    }
    
    // Buscar en blocked queue
    if ((p = find_in_queue(&blocked_q, pid))) {
        return p;
    }
    
    // Buscar en finished queue
    if ((p = find_in_queue(&finished_q, pid))) {
        return p;
    }
    
    // Verificar si es el proceso actual
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
    sched_crit_enter();
    
    process_close_fds(p);
    
    if (p->is_foreground && p->parent && p->parent->state != PROCESS_STATE_FINISHED) {
        p->parent->is_foreground = 1;
    }
    
    // Remover de donde esté antes de mover a finished
    if (p->state == PROCESS_STATE_READY) {
        process_queue_remove(&ready_queues[p->priority], p);
    } else if (p->state == PROCESS_STATE_BLOCKED) {
        process_queue_remove(&blocked_q, p);
    }
    
    p->state = PROCESS_STATE_FINISHED;
    
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

    // Si el proceso está READY, necesitamos moverlo de una ready queue a otra
    if (p->state == PROCESS_STATE_READY) {
        process_queue_remove(&ready_queues[oldp], p);
    }

    p->priority = clamped;
    p->base_priority = clamped;
    p->wait_ticks = 0;  

    if (p->state == PROCESS_STATE_READY) {
        ready_queue_push(p);
    }
    
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
        current->state = PROCESS_STATE_BLOCKED;
        need_resched = 1;
        sched_crit_exit();
        return 1;
    }

    // Para cualquier otro proceso en estado READY, bloquearlo
    if (p->state == PROCESS_STATE_READY) {
        // Remover de la ready queue específica donde debería estar
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
    
    int i;
    for (i = 0; i < PROCESS_NAME_MAX_LEN && p->name[i] != '\0'; i++) {
        buffer[*count].name[i] = p->name[i];
    }
    buffer[*count].name[i] = '\0';
    
    buffer[*count].state = p->state;
    buffer[*count].priority = p->priority;
    buffer[*count].rsp = p->rsp;
    buffer[*count].rbp = p->rbp;
    buffer[*count].foreground = p->is_foreground;
    (*count)++;
}

uint64_t scheduler_list_all_processes(process_info_t *buffer, uint64_t max_count) {
    if (!buffer || max_count == 0) {
        return 0;
    }

    sched_crit_enter();
    uint64_t count = 0;

    for (int pr = PROCESS_PRIORITY_MIN; pr <= PROCESS_PRIORITY_MAX; ++pr) {
        for (process_t *p = ready_queues[pr].head; p && count < max_count; p = p->queue_next) {
            add_process_to_list(p, buffer, &count, max_count);
        }
    }

    for (process_t *p = blocked_q.head; p && count < max_count; p = p->queue_next) {
        add_process_to_list(p, buffer, &count, max_count);
    }

    for (process_t *p = finished_q.head; p && count < max_count; p = p->queue_next) {
        add_process_to_list(p, buffer, &count, max_count);
    }

    if (current && count < max_count) {
        int already_added = 0;
        for (uint64_t i = 0; i < count; i++) {
            if (buffer[i].pid == current->pid) {
                already_added = 1;
                buffer[i].state = current->state;
                buffer[i].rsp = current->rsp;
                buffer[i].rbp = current->rbp;
                buffer[i].foreground = current->is_foreground;
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

uint64_t scheduler_get_foreground_pid(void) {
    sched_crit_enter();
    
    if (current && current != idle_p && current->is_foreground) {
        uint64_t pid = current->pid;
        sched_crit_exit();
        return pid;
    }
    
    for (int pr = PROCESS_PRIORITY_MIN; pr <= PROCESS_PRIORITY_MAX; ++pr) {
        for (process_t *p = ready_queues[pr].head; p; p = p->queue_next) {
            if (p->is_foreground) {
                uint64_t pid = p->pid;
                sched_crit_exit();
                return pid;
            }
        }
    }
    
    for (process_t *p = blocked_q.head; p; p = p->queue_next) {
        if (p->is_foreground) {
            uint64_t pid = p->pid;
            sched_crit_exit();
            return pid;
        }
    }
    
    sched_crit_exit();
    return 0;
}
