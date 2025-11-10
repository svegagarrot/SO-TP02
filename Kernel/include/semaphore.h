#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <process.h>
#include <stdint.h>

typedef struct semaphore_t semaphore_t;

// Kernel-facing API
uint64_t sem_alloc(int initial_value);
semaphore_t *sem_get_by_id(uint64_t id);
int sem_open_by_id(uint64_t id);
int sem_close_by_id(uint64_t id);
int sem_wait_by_id(uint64_t id);
int sem_signal_by_id(uint64_t id);
int sem_set_by_id(uint64_t id, int newval);
int sem_get_value_by_id(uint64_t id, int *out);

#endif
