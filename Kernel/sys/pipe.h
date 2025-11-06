#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>
#include <stddef.h>

typedef struct pipe_t pipe_t;

// Kernel-facing API
void pipe_system_init(void);  // Inicializar sistema de pipes
uint64_t pipe_create(void);
int pipe_open_by_id(uint64_t id);
int pipe_close_by_id(uint64_t id, int is_writer);
int pipe_release_ref(uint64_t id);  // Liberar una referencia sin marcar writers_closed
pipe_t *pipe_get_by_id(uint64_t id);
uint64_t pipe_get_id(pipe_t *p);  // Obtener ID de un pipe (para usar en process_destroy)

// Funciones de lectura/escritura (para futura integración)
int pipe_read(pipe_t *p, char *buffer, size_t count);
int pipe_write(pipe_t *p, const char *buffer, size_t count);

#endif

