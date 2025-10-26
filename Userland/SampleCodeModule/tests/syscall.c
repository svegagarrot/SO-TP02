#include <stdint.h>
#include "../include/syscall.h"

int64_t my_getpid() {
  return (int64_t)sys_getpid();
}


int64_t my_nice(uint64_t pid, uint64_t newPrio) {
  return (int64_t)sys_set_priority(pid, newPrio);
}


int64_t my_sem_open(char *sem_id, uint64_t initialValue) {
  return 0;
}

int64_t my_sem_wait(char *sem_id) {
  return 0;
}

int64_t my_sem_post(char *sem_id) {
  return 0;
}

int64_t my_sem_close(char *sem_id) {
  return 0;
}

int64_t my_yield() {
  return 0;
}

int64_t my_wait(int64_t pid) {
  return (int64_t)sys_wait((uint64_t)pid);
}