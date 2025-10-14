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
void scheduler_unblock_process(process_t *p);
void scheduler_finish_current(void);
process_t *scheduler_collect_finished(void);
int scheduler_kill_by_pid(uint64_t pid);
int scheduler_unblock_by_pid(uint64_t pid);

#endif
