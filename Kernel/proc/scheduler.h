#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "process.h"

uint64_t schedule(uint64_t current_rsp);
void init_scheduler(void);

process_t *scheduler_current_process(void);
void scheduler_add_process(process_t *process);
process_t *scheduler_spawn_process(const char *name,
                                   process_entry_point_t entry_point,
                                   void *entry_arg,
                                   int priority,
                                   process_mode_t mode,
                                   process_type_t type,
                                   process_t *parent);
void scheduler_block_current(void);
void scheduler_unblock_process(process_t *process);
void scheduler_finish_current(void);
process_t *scheduler_collect_finished(void);
process_t *scheduler_find_by_pid(uint64_t pid);
int scheduler_unblock_by_pid(uint64_t pid);
int scheduler_kill_by_pid(uint64_t pid);

#endif
