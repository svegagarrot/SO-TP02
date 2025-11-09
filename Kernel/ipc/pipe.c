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
    int readers;  // Contador de lectores
    int writers;  // Contador de escritores
    uint64_t sem_items;   // Semáforo para items disponibles (inicialmente = 0)
    uint64_t sem_spaces;  // Semáforo para espacios disponibles (inicialmente = PIPE_BUFFER_SIZE)
    uint64_t mutex;       // Mutex para exclusión mutua (inicialmente = 1)
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
        pipes[i].sem_items = 0;
        pipes[i].sem_spaces = 0;
        pipes[i].mutex = 0;
        pipes[i].read_pos = 0;
        pipes[i].write_pos = 0;
        pipes[i].count = 0;
        pipes[i].readers = 0;
        pipes[i].writers = 0;
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
            // sem_items: inicialmente 0 (no hay datos disponibles)
            p->sem_items = sem_alloc(0);
            if (p->sem_items == 0) {
                return 0; // Fallo al crear semáforo
            }
            
            // sem_spaces: inicialmente PIPE_BUFFER_SIZE (espacios vacíos disponibles)
            p->sem_spaces = sem_alloc(PIPE_BUFFER_SIZE);
            if (p->sem_spaces == 0) {
                sem_close_by_id(p->sem_items);
                return 0; // Fallo al crear semáforo
            }
            
            // mutex: inicialmente 1 (disponible)
            p->mutex = sem_alloc(1);
            if (p->mutex == 0) {
                sem_close_by_id(p->sem_items);
                sem_close_by_id(p->sem_spaces);
                return 0; // Fallo al crear semáforo
            }
            
            // Inicializar campos
            p->id = next_pipe_id++;
            p->read_pos = 0;
            p->write_pos = 0;
            p->count = 0;
            p->readers = 0;
            p->writers = 0;
            p->used = 1;
            
            return p->id;
        }
    }
    return 0; // No hay slots disponibles
}

int pipe_open_by_id(uint64_t id, int is_writer) {
    pipe_t *p = pipe_get_by_id(id);
    if (!p) return 0;
    
    // Incrementar contador de lectores o escritores (usar mutex para proteger)
    sem_wait_by_id(p->mutex);
    if (is_writer) {
        p->writers++;
    } else {
        p->readers++;
    }
    sem_signal_by_id(p->mutex);
    
    return 1;
}


int pipe_close_by_id(uint64_t id, int is_writer) {
    pipe_t *p = pipe_get_by_id(id);
    if (!p) return 0;
    
    sem_wait_by_id(p->mutex);
    
    // Decrementar contador de lectores o escritores
    int was_last_writer = 0;
    
    if (is_writer) {
        if (p->writers > 0) {
            p->writers--;
            was_last_writer = (p->writers == 0);
        }
    } else {
        if (p->readers > 0) {
            p->readers--;
        }
    }
    
    // Verificar si debemos destruir el pipe (no hay lectores ni escritores)
    int should_destroy = (p->readers == 0 && p->writers == 0);
    
    if (should_destroy) {
        // Guardar IDs de semáforos antes de cerrarlos
        uint64_t items_id = p->sem_items;
        uint64_t spaces_id = p->sem_spaces;
        uint64_t mutex_id = p->mutex;
        
        // Marcar como no usado antes de liberar el mutex
        p->used = 0;
        p->id = 0;
        p->sem_items = 0;
        p->sem_spaces = 0;
        p->mutex = 0;
        p->read_pos = 0;
        p->write_pos = 0;
        p->count = 0;
        p->readers = 0;
        p->writers = 0;
        
        // Liberar mutex (ya no lo necesitamos)
        sem_signal_by_id(mutex_id);
        
        // Cerrar semáforos (después de liberar el mutex)
        sem_close_by_id(items_id);
        sem_close_by_id(spaces_id);
        sem_close_by_id(mutex_id);
    } else {
        // Si el último escritor cerró, despertar lectores bloqueados
        if (was_last_writer) {
            // Señalizar sem_items para despertar lectores bloqueados
            // Señalizamos una vez por cada lector activo (máximo razonable)
            // Esto es mejor que un loop arbitrario de 100
            int readers_to_wake = p->readers;
            if (readers_to_wake > 0) {
                // Señalizar una vez por cada lector (hasta un máximo razonable)
                for (int i = 0; i < readers_to_wake && i < 100; i++) {
                    sem_signal_by_id(p->sem_items);
                }
            }
        }
        
        sem_signal_by_id(p->mutex);
    }
    
    return 1;
}

int pipe_read(pipe_t *p, char *buffer, size_t count) {
    if (!p || !buffer || count == 0) {
        return 0;
    }
    
    size_t bytes_read = 0;

    while (bytes_read < count) {
        // Adquirir mutex primero para verificar estado
        if (!sem_wait_by_id(p->mutex)) {
            break;
        }

        // Verificar EOF: no hay escritores y no hay datos en el buffer
        if (p->count == 0 && p->writers == 0) {
            sem_signal_by_id(p->mutex);
            // Si ya leímos algo, retornar lo que leímos
            // Si no leímos nada, esto es EOF (retornar 0)
            break;
        }

        // Leer datos disponibles
        if (p->count > 0) {
            buffer[bytes_read] = p->buffer[p->read_pos];
            p->read_pos = (p->read_pos + 1) % PIPE_BUFFER_SIZE;
            p->count--;
            bytes_read++;
            
            sem_signal_by_id(p->mutex);
            sem_signal_by_id(p->sem_spaces);  // Señalar que hay un espacio disponible
        } else {
            // No hay datos pero hay escritores
            // Si ya leímos algunos bytes, retornar los bytes parciales
            // Esto permite que el lector procese los datos disponibles mientras
            // el escritor está inactivo (ej: en sleep())
            if (bytes_read > 0) {
                sem_signal_by_id(p->mutex);
                break;  // Retornar los bytes leídos hasta el momento
            }
            
            // No hemos leído nada todavía, esperar por datos
            sem_signal_by_id(p->mutex);
            
            // Esperar por datos disponibles
            if (!sem_wait_by_id(p->sem_items)) {
                break;
            }
            // Volver al inicio del loop para intentar leer de nuevo
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
        // Adquirir mutex para verificar estado
        if (!sem_wait_by_id(p->mutex)) {
            break;
        }

        // Verificar si hay lectores antes de intentar escribir
        if (p->readers == 0) {
            // No hay lectores, retornar 0 (equivalente a EPIPE)
            sem_signal_by_id(p->mutex);
            // Si ya escribimos algo, retornar lo que escribimos
            // Si no escribimos nada, retornar 0 (error)
            break;
        }

        // Hay lectores, verificar si hay espacio disponible
        if (p->count < PIPE_BUFFER_SIZE) {
            // Hay espacio, escribir directamente
            p->buffer[p->write_pos] = buffer[bytes_written];
            p->write_pos = (p->write_pos + 1) % PIPE_BUFFER_SIZE;
            p->count++;
            bytes_written++;

            sem_signal_by_id(p->mutex);
            sem_signal_by_id(p->sem_items);  // Señalar que hay un item disponible
        } else {
            // No hay espacio, liberar mutex y esperar por espacio
            sem_signal_by_id(p->mutex);
            
            // Esperar por espacio disponible
            if (!sem_wait_by_id(p->sem_spaces)) {
                break;
            }
            // Volver al inicio del loop para intentar escribir de nuevo
        }
    }
    
    return (int)bytes_written;
}
