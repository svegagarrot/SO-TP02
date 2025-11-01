#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <stdint.h>
#include "process.h"

void init_scheduler(void);
uint64_t schedule(uint64_t current_rsp);

process_t *scheduler_current_process(void);
void scheduler_add_process(process_t *p);

process_t *scheduler_spawn_process(const char *name,
                                   process_entry_point_t entry_point,
                                   void *entry_arg,
                                   process_t *parent);

void scheduler_block_current(void);
void scheduler_yield_current(void);
void scheduler_unblock_process(process_t *p);
void scheduler_finish_current(void);
process_t *scheduler_collect_finished(void);
int scheduler_kill_by_pid(uint64_t pid);
int scheduler_unblock_by_pid(uint64_t pid);
int scheduler_block_by_pid(uint64_t pid);

process_t *scheduler_find_by_pid(uint64_t pid);
int scheduler_set_priority(uint64_t pid, uint8_t new_priority);

// Función para listar todos los procesos del sistema
uint64_t scheduler_list_all_processes(process_info_t *buffer, uint64_t max_count);

#endif
