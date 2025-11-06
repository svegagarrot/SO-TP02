#include "pipe.h"
#include "semaphore.h"
#include "process.h"
#include "scheduler.h"
#include "lib.h"
#include <stddef.h>
#include <string.h>

#define MAX_PIPES 64
#define PIPE_BUFFER_SIZE 4096

struct pipe_t {
    uint64_t id;
    char buffer[PIPE_BUFFER_SIZE];
    size_t read_pos;
    size_t write_pos;
    size_t count;
    int refcount;
    int writers_closed;  // Flag para indicar que no hay más escritores
    uint64_t sem_readers_id;   // Semáforo para lectores (inicialmente = 0)
    uint64_t sem_writers_id;   // Semáforo para escritores (inicialmente = PIPE_BUFFER_SIZE)
    uint64_t mutex_id;          // Mutex para exclusión mutua (inicialmente = 1)
    int used;
};

static pipe_t pipes[MAX_PIPES];
static uint64_t next_pipe_id = 1;

void pipe_system_init(void) {
    // Inicializar array de pipes (aunque los arrays estáticos ya se inicializan a 0,
    // esto asegura que todo esté en un estado conocido)
    for (int i = 0; i < MAX_PIPES; i++) {
        pipes[i].used = 0;
        pipes[i].id = 0;
        pipes[i].sem_readers_id = 0;
        pipes[i].sem_writers_id = 0;
        pipes[i].mutex_id = 0;
        pipes[i].read_pos = 0;
        pipes[i].write_pos = 0;
        pipes[i].count = 0;
        pipes[i].refcount = 0;
    }
    next_pipe_id = 1;
}

static pipe_t *find_pipe_by_id(uint64_t id) {
    for (int i = 0; i < MAX_PIPES; ++i) {
        if (pipes[i].used && pipes[i].id == id) {
            return &pipes[i];
        }
    }
    return NULL;
}

pipe_t *pipe_get_by_id(uint64_t id) {
    if (id == 0) return NULL;
    return find_pipe_by_id(id);
}

uint64_t pipe_get_id(pipe_t *p) {
    if (!p) return 0;
    return p->id;
}

uint64_t pipe_create(void) {
    // Buscar un slot libre
    for (int i = 0; i < MAX_PIPES; ++i) {
        if (!pipes[i].used) {
            pipe_t *p = &pipes[i];
            
            // Inicializar estructura
            memset(p, 0, sizeof(pipe_t));
            
            // Crear semáforos para sincronización
            // sem_readers: inicialmente 0 (no hay datos disponibles)
            p->sem_readers_id = sem_alloc(0);
            if (p->sem_readers_id == 0) {
                return 0; // Fallo al crear semáforo
            }
            
            // sem_writers: inicialmente PIPE_BUFFER_SIZE (espacios vacíos disponibles)
            p->sem_writers_id = sem_alloc(PIPE_BUFFER_SIZE);
            if (p->sem_writers_id == 0) {
                sem_close_by_id(p->sem_readers_id);
                return 0; // Fallo al crear semáforo
            }
            
            // mutex: inicialmente 1 (disponible)
            p->mutex_id = sem_alloc(1);
            if (p->mutex_id == 0) {
                sem_close_by_id(p->sem_readers_id);
                sem_close_by_id(p->sem_writers_id);
                return 0; // Fallo al crear semáforo
            }
            
            // Inicializar campos
            p->id = next_pipe_id++;
            p->read_pos = 0;
            p->write_pos = 0;
            p->count = 0;
            p->refcount = 1;
            p->writers_closed = 0;  // Inicialmente hay escritores
            p->used = 1;
            
            return p->id;
        }
    }
    return 0; // No hay slots disponibles
}

int pipe_open_by_id(uint64_t id) {
    pipe_t *p = pipe_get_by_id(id);
    if (!p) return 0;
    
    // Incrementar refcount (usar mutex para proteger)
    sem_wait_by_id(p->mutex_id);
    p->refcount++;
    sem_signal_by_id(p->mutex_id);
    
    return 1;
}

// Función para liberar una referencia sin marcar writers_closed
// Solo debe usarse cuando se libera un file descriptor, no cuando se cierra explícitamente
int pipe_release_ref(uint64_t id) {
    pipe_t *p = pipe_get_by_id(id);
    if (!p) return 0;
    
    // Decrementar refcount (usar mutex para proteger)
    sem_wait_by_id(p->mutex_id);
    p->refcount--;
    int ref = p->refcount;
    
    if (ref <= 0) {
        // Guardar IDs de semáforos antes de cerrarlos
        uint64_t readers_id = p->sem_readers_id;
        uint64_t writers_id = p->sem_writers_id;
        uint64_t mutex_id = p->mutex_id;
        
        // Marcar como no usado antes de liberar el mutex
        p->used = 0;
        p->id = 0;
        p->sem_readers_id = 0;
        p->sem_writers_id = 0;
        p->mutex_id = 0;
        p->read_pos = 0;
        p->write_pos = 0;
        p->count = 0;
        
        // Liberar mutex (ya no lo necesitamos)
        sem_signal_by_id(mutex_id);
        
        // Cerrar semáforos (después de liberar el mutex)
        sem_close_by_id(readers_id);
        sem_close_by_id(writers_id);
        sem_close_by_id(mutex_id);
    } else {
        // Todavía hay referencias, solo liberar el mutex
        // NO marcar writers_closed porque otros procesos pueden estar usando el pipe
        sem_signal_by_id(p->mutex_id);
    }
    
    return 1;
}

int pipe_close_by_id(uint64_t id) {
    pipe_t *p = pipe_get_by_id(id);
    if (!p) return 0;
    
    // Decrementar refcount (usar mutex para proteger)
    sem_wait_by_id(p->mutex_id);
    p->refcount--;
    int ref = p->refcount;
    
    if (ref <= 0) {
        // Guardar IDs de semáforos antes de cerrarlos
        uint64_t readers_id = p->sem_readers_id;
        uint64_t writers_id = p->sem_writers_id;
        uint64_t mutex_id = p->mutex_id;
        
        // Marcar como no usado antes de liberar el mutex
        p->used = 0;
        p->id = 0;
        p->sem_readers_id = 0;
        p->sem_writers_id = 0;
        p->mutex_id = 0;
        p->read_pos = 0;
        p->write_pos = 0;
        p->count = 0;
        
        // Liberar mutex (ya no lo necesitamos)
        sem_signal_by_id(mutex_id);
        
        // Cerrar semáforos (después de liberar el mutex)
        sem_close_by_id(readers_id);
        sem_close_by_id(writers_id);
        sem_close_by_id(mutex_id);
    } else {
        // Todavía hay referencias, marcar escritores como cerrados y despertar lectores
        p->writers_closed = 1;
        // Despertar a todos los lectores bloqueados señalando el semáforo múltiples veces
        for (int i = 0; i < 100; i++) {  // Número arbitrario grande
            sem_signal_by_id(p->sem_readers_id);
        }
        sem_signal_by_id(p->mutex_id);
    }
    
    return 1;
}

int pipe_read(pipe_t *p, char *buffer, size_t count) {
    if (!p || !buffer || count == 0) {
        return 0;
    }
    
    size_t bytes_read = 0;

    while (bytes_read < count) {
        // Esperar por un byte disponible
        if (!sem_wait_by_id(p->sem_readers_id)) {
            break;
        }

        if (!sem_wait_by_id(p->mutex_id)) {
            // Devolver el signal que consumimos
            sem_signal_by_id(p->sem_readers_id);
            break;
        }

        // Verificar si los escritores cerraron y no hay datos
        if (p->count == 0 && p->writers_closed) {
            sem_signal_by_id(p->mutex_id);
            sem_signal_by_id(p->sem_readers_id);  // Devolver el signal del semáforo
            // Si ya leímos algo, retornar lo que leímos
            // Si no leímos nada, esto es EOF
            break;
        }

        // Debe haber al menos un dato si llegamos aquí
        if (p->count > 0) {
            buffer[bytes_read] = p->buffer[p->read_pos];
            p->read_pos = (p->read_pos + 1) % PIPE_BUFFER_SIZE;
            p->count--;
            bytes_read++;
            
            sem_signal_by_id(p->mutex_id);
            sem_signal_by_id(p->sem_writers_id);
            
            // Retornar después de leer al menos algo
            // Esto evita bloquear esperando más datos
            if (bytes_read > 0) {
                break;
            }
        } else {
            // No debería pasar, pero por seguridad
            sem_signal_by_id(p->mutex_id);
            break;
        }
    }
    
    return (int)bytes_read;
}

int pipe_write(pipe_t *p, const char *buffer, size_t count) {
    if (!p || !buffer || count == 0) {
        return 0;
    }
    
    size_t bytes_written = 0;

    while (bytes_written < count) {
        if (!sem_wait_by_id(p->sem_writers_id)) {
            break;
        }

        if (!sem_wait_by_id(p->mutex_id)) {
            sem_signal_by_id(p->sem_writers_id);  // Devolver el signal del semáforo
            break;
        }

        // Verificar que realmente hay espacio disponible
        if (p->count < PIPE_BUFFER_SIZE) {
            p->buffer[p->write_pos] = buffer[bytes_written];
            p->write_pos = (p->write_pos + 1) % PIPE_BUFFER_SIZE;
            p->count++;
            bytes_written++;

            sem_signal_by_id(p->mutex_id);
            sem_signal_by_id(p->sem_readers_id);
        } else {
            // No debería pasar si los semáforos están bien sincronizados
            sem_signal_by_id(p->mutex_id);
            sem_signal_by_id(p->sem_writers_id);  // Devolver el signal
            break;
        }
    }
    
    return (int)bytes_written;
}
