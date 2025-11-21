// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <interrupts.h>
#include <lib.h>
#include <process.h>
#include <scheduler.h>
#include <semaphore.h>
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
	int random_dequeue; // flag para activar dequeue aleatorio
} semaphore_t;

static semaphore_t semaphores[MAX_SEMAPHORES];
static uint64_t next_sem_id = 1;

static semaphore_t *find_slot_by_id(uint64_t id) {
	for (int i = 0; i < MAX_SEMAPHORES; ++i) {
		if (semaphores[i].used && semaphores[i].id == id)
			return &semaphores[i];
	}
	return NULL;
}

semaphore_t *sem_get_by_id(uint64_t id) {
	if (id == 0)
		return NULL;
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
			s->random_dequeue = 0; // por defecto FIFO
			return s->id;
		}
	}
	return 0;
}

int sem_open_by_id(uint64_t id) {
	semaphore_t *s = sem_get_by_id(id);
	if (!s)
		return 0;
	acquire(&s->lock);
	s->refcount++;
	release(&s->lock);
	return 1;
}

int sem_close_by_id(uint64_t id) {
	semaphore_t *s = sem_get_by_id(id);
	if (!s)
		return 0;
	acquire(&s->lock);
	s->refcount--;
	int ref = s->refcount;
	release(&s->lock);
	if (ref <= 0) {
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
	}
	else {
		s->wait_tail->sem_waiter_next = p;
		s->wait_tail = p;
	}
}

static process_t *dequeue_waiter_random(semaphore_t *s) {
	if (!s->wait_head)
		return NULL;
	
	// contar cuantos procesos estan esperando
	int count = 0;
	process_t *p = s->wait_head;
	while (p) {
		count++;
		p = p->sem_waiter_next;
	}
	
	if (count == 0)
		return NULL;
	
	// si solo hay uno, devolverlo directamente
	if (count == 1) {
		p = s->wait_head;
		s->wait_head = s->wait_tail = NULL;
		p->sem_waiter_next = NULL;
		return p;
	}
	
	// generar un indice pseudo-aleatorio 
	extern uint64_t ticks_elapsed(void);
	extern process_t *scheduler_current_process(void);
	
	static uint64_t random_state = 123456789UL;
	
	uint64_t seed = ticks_elapsed();
	
	// mezclar con el PID del proceso actual si existe
	process_t *current = scheduler_current_process();
	if (current) {
		seed ^= (current->pid << 16);
		seed ^= (uint64_t)current->rsp;
	}
	
	// mezclar con la dirección del semáforo
	seed ^= (uint64_t)s;
	
	// mezclar con el estado anterior
	seed ^= random_state;
	
	seed = (seed * 1103515245UL + 12345);
	random_state = seed; // actualizar estado para la prox vez
	
	int target_index = (seed >> 16) % count; // usar bits medios para mejor dsitribucioin
	
	// ir hasta el proceso target_index
	process_t *prev = NULL;
	p = s->wait_head;
	for (int i = 0; i < target_index; i++) {
		prev = p;
		p = p->sem_waiter_next;
	}
	
	// borrar el proceso de la lista
	if (prev) {
		prev->sem_waiter_next = p->sem_waiter_next;
	} else {
		s->wait_head = p->sem_waiter_next;
	}
	
	if (p == s->wait_tail) {
		s->wait_tail = prev;
	}
	
	p->sem_waiter_next = NULL;
	return p;
}

static process_t *dequeue_waiter(semaphore_t *s) {
	// usar dequeue aleatorio solo si esta activado para este sem
	if (s->random_dequeue) {
		return dequeue_waiter_random(s);
	}
	
	// deque fifo normal
	process_t *p = s->wait_head;
	if (!p)
		return NULL;
	s->wait_head = p->sem_waiter_next;
	if (!s->wait_head)
		s->wait_tail = NULL;
	p->sem_waiter_next = NULL;
	return p;
}

int sem_wait_by_id(uint64_t id) {
	semaphore_t *s = sem_get_by_id(id);
	if (!s)
		return 0;

	process_t *me = scheduler_current_process();
	if (!me)
		return 0;

	acquire(&s->lock);
	if (s->value > 0) {
		s->value--;
		release(&s->lock);
		return 1;
	}
	me->waiting_on_sem = s->id;
	enqueue_waiter(s, me);
	release(&s->lock);

	scheduler_block_current();
	scheduler_yield_current();
	return 1;
}

int sem_signal_by_id(uint64_t id) {
	semaphore_t *s = sem_get_by_id(id);
	if (!s)
		return 0;

	acquire(&s->lock);
	process_t *w = dequeue_waiter(s);
	if (w) {
		w->waiting_on_sem = 0;
		scheduler_unblock_process(w);
	}
	else {
		s->value++;
	}
	release(&s->lock);
	return 1;
}

int sem_set_by_id(uint64_t id, int newval) {
	semaphore_t *s = sem_get_by_id(id);
	if (!s)
		return 0;
	if (newval < 0)
		return 0;

	acquire(&s->lock);
	int awaken = 0;
	while (awaken < newval) {
		process_t *w = dequeue_waiter(s);
		if (!w)
			break;
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
	if (!s || !out)
		return 0;
	acquire(&s->lock);
	*out = s->value;
	release(&s->lock);
	return 1;
}

int sem_set_random_dequeue(uint64_t id, int enable) {
	semaphore_t *s = sem_get_by_id(id);
	if (!s)
		return 0;
	acquire(&s->lock);
	s->random_dequeue = enable ? 1 : 0;
	release(&s->lock);
	return 1;
}
