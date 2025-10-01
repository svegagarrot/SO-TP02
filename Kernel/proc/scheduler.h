#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

uint64_t schedule(uint64_t current_rsp);
void init_scheduler(void);
void idle_process(void);

#endif