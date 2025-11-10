// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <semaphore.h>
#include <process.h>
#include <scheduler.h>
#include <interrupts.h>
#include <lib.h>
#include <naiveConsole.h>
#include <stddef.h>

#define MAX_SEMAPHORES 64

typedef volatile uint8_t lock_t;

typedef struct semaphore_t {
    uint64_t id;
    int value;
    int refcount;
    lock_t lock;
    process_t *wait_head;
    process_t *wait_tail;
    int used;
} semaphore_t;

static semaphore_t semaphores[MAX_SEMAPHORES];
static uint64_t next_sem_id = 1;

static semaphore_t *find_slot_by_id(uint64_t id) {
    for (int i = 0; i < MAX_SEMAPHORES; ++i) {
        if (semaphores[i].used && semaphores[i].id == id) return &semaphores[i];
    }
    return NULL;
}

semaphore_t *sem_get_by_id(uint64_t id) {
    if (id == 0) return NULL;
    return find_slot_by_id(id);
}

uint64_t sem_alloc(int initial_value) {
    for (int i = 0; i < MAX_SEMAPHORES; ++i) {
        if (!semaphores[i].used) {
            semaphore_t *s = &semaphores[i];
            s->used = 1;
            s->id = next_sem_id++;
            s->value = initial_value;
            s->refcount = 1;
            s->lock = 0;
            s->wait_head = s->wait_tail = NULL;
            return s->id;
        }
    }
    return 0;
}

int sem_open_by_id(uint64_t id) {
    semaphore_t *s = sem_get_by_id(id);
    if (!s) return 0;
    acquire(&s->lock);
    s->refcount++;
    release(&s->lock);
    return 1;
}

int sem_close_by_id(uint64_t id) {
    semaphore_t *s = sem_get_by_id(id);
    if (!s) return 0;
    acquire(&s->lock);
    s->refcount--;
    int ref = s->refcount;
    release(&s->lock);
    if (ref <= 0) {
        // free: ensure no waiters
        acquire(&s->lock);
        s->used = 0;
        s->wait_head = s->wait_tail = NULL;
        release(&s->lock);
    }
    return 1;
}

static void enqueue_waiter(semaphore_t *s, process_t *p) {
    p->sem_waiter_next = NULL;
    if (!s->wait_head) {
        s->wait_head = s->wait_tail = p;
    } else {
        s->wait_tail->sem_waiter_next = p;
        s->wait_tail = p;
    }
}

static process_t *dequeue_waiter(semaphore_t *s) {
    process_t *p = s->wait_head;
    if (!p) return NULL;
    s->wait_head = p->sem_waiter_next;
    if (!s->wait_head) s->wait_tail = NULL;
    p->sem_waiter_next = NULL;
    return p;
}

int sem_wait_by_id(uint64_t id) {
    semaphore_t *s = sem_get_by_id(id);
    if (!s) return 0;

    process_t *me = scheduler_current_process();
    if (!me) return 0;

    acquire(&s->lock);
    if (s->value > 0) {
        s->value--;
        release(&s->lock);
        return 1;
    }
    // enqueue and mark waiting_on_sem
    me->waiting_on_sem = s->id;
    enqueue_waiter(s, me);
    release(&s->lock);

    // Marcar bloqueado y forzar cambio inmediato para que no continúe ejecutando
    // hasta que el scheduler efectivamente lo mueva a la cola de bloqueados.
    scheduler_block_current();
    scheduler_yield_current();
    return 1;
}

int sem_signal_by_id(uint64_t id) {
    semaphore_t *s = sem_get_by_id(id);
    if (!s) return 0;

    acquire(&s->lock);
    process_t *w = dequeue_waiter(s);
    if (w) {
        w->waiting_on_sem = 0;
        // Unblock using scheduler helper
        scheduler_unblock_process(w);
    } else {
        s->value++;
    }
    release(&s->lock);
    return 1;
}

int sem_set_by_id(uint64_t id, int newval) {
    semaphore_t *s = sem_get_by_id(id);
    if (!s) return 0;
    if (newval < 0) return 0;

    acquire(&s->lock);
    // wake up up to newval waiters
    int awaken = 0;
    while (awaken < newval) {
        process_t *w = dequeue_waiter(s);
        if (!w) break;
        w->waiting_on_sem = 0;
        scheduler_unblock_process(w);
        awaken++;
    }
    s->value = newval - awaken;
    release(&s->lock);
    return 1;
}

int sem_get_value_by_id(uint64_t id, int *out) {
    semaphore_t *s = sem_get_by_id(id);
    if (!s || !out) return 0;
    acquire(&s->lock);
    *out = s->value;
    release(&s->lock);
    return 1;
}
