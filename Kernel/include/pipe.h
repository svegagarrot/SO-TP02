#ifndef PIPE_H
#define PIPE_H

#include <stddef.h>
#include <stdint.h>

typedef struct pipe_t pipe_t;


void pipe_system_init(void);
uint64_t pipe_create(void);
int pipe_open_by_id(uint64_t id, int is_writer);
int pipe_close_by_id(uint64_t id, int is_writer);
pipe_t *pipe_get_by_id(uint64_t id);
uint64_t pipe_get_id(pipe_t *p);

int pipe_read(pipe_t *p, char *buffer, size_t count);
int pipe_write(pipe_t *p, const char *buffer, size_t count);

#endif
