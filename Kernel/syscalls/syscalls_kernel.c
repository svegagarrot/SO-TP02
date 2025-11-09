#include <syscalls_lib.h>
#include <keyboardDriver.h>
#include <videoDriver.h>
#include <stddef.h>
#include <lib.h>
#include <interrupts.h>
#include <time.h>
#include <keystate.h>
#include <process.h>
#include <scheduler.h>
#include <mm.h>
#include <string.h>
#include <semaphore.h>
#include <pipe.h>

#define STDIN 0
#define STDOUT 1

uint64_t syscall_read(int fd, char *buffer, int count) {
    if (buffer == NULL || count <= 0 || fd < 0 || fd >= MAX_FDS) {
        return 0;
    }

    process_t *current_process = scheduler_current_process();
    if (!current_process) {
        return 0;
    }

    // Consultar tabla de file descriptors
    fd_entry_t *fd_entry = &current_process->fds[fd];
    
    // Leer según el tipo de FD
    if (fd_entry->type == FD_TYPE_TERMINAL) {
        // Leer de terminal (STDIN)
        if (fd != STDIN) {
            return 0;  // Solo FD 0 puede leer de terminal
        }

        // Verificar si el proceso actual está en foreground
        if (!current_process->is_foreground) {
            // Proceso en background no puede leer del teclado
            // Lo bloqueamos automáticamente
            scheduler_block_by_pid(current_process->pid);
            // Después de bloquearse y despertar, devolvemos 0 (no hay datos)
            return 0;
        }

        uint64_t read = 0;
        while (read < (uint64_t)count) {
            char c = keyboard_read_getchar();
            if (c == 0) {
                keyboard_wait_for_char();
                continue;
            }
            
            // Detectar EOF (Ctrl+D = 0x04)
            if (c == 0x04) {
                // Si no hemos leído nada, retornar 0 (EOF)
                // Si ya leímos algo, retornar lo que tenemos
                break;
            }
            
            buffer[read++] = c;
            if (c == '\n') break;
        }
        return read;
    }
    else if (fd_entry->type == FD_TYPE_PIPE_READ) {
        // Leer de pipe
        if (!fd_entry->pipe) {
            return 0;
        }
        return pipe_read(fd_entry->pipe, buffer, count);
    }
    else {
        // FD_TYPE_PIPE_WRITE: no se puede leer de un pipe de escritura
        return 0;
    }
}


uint64_t syscall_write(int fd, const char * buffer, int count) {
    if (buffer == NULL || count <= 0 || fd < 0 || fd >= MAX_FDS) {
        return 0;
    }

    process_t *current_process = scheduler_current_process();
    if (!current_process) {
        return 0;
    }

    // Consultar tabla de file descriptors
    fd_entry_t *fd_entry = &current_process->fds[fd];
    
    // Escribir según el tipo de FD
    if (fd_entry->type == FD_TYPE_TERMINAL) {
        // Escribir a terminal (STDOUT)
        if (fd != STDOUT) {
            return 0;  // Solo FD 1 puede escribir a terminal
        }
        
        for (int i = 0; i < count; i++) {
            video_putChar(buffer[i], FOREGROUND_COLOR, BACKGROUND_COLOR);
        }
        
        return count;
    }
    else if (fd_entry->type == FD_TYPE_PIPE_WRITE) {
        // Escribir a pipe
        if (!fd_entry->pipe) {
            return 0;
        }
        return pipe_write(fd_entry->pipe, buffer, count);
    }
    else {
        // FD_TYPE_PIPE_READ: no se puede escribir a un pipe de lectura
        return 0;
    }
}

uint64_t syscall_clearScreen() {
    video_clearScreen();
    return 1;
}

uint64_t syscall_sleep(int duration) {

    const uint32_t HZ = 18;
    uint64_t start     = ticks_elapsed();
    uint64_t wait_tics = (duration * HZ + 999) / 1000;
    uint64_t target    = start + wait_tics;


    while (ticks_elapsed() < target) {
        _hlt();
    }
    return 0;
}

uint64_t syscall_video_putChar(uint64_t c, uint64_t fg, uint64_t bg, uint64_t unused1, uint64_t unused2) {
    video_putChar((char)c, fg, bg);
    return 0;
}

uint64_t syscall_shutdown(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    outw(0x604, 0x2000);
    
    _cli();
    
    while(1) {
        _hlt();
    }
    
    return 0; 
}

uint64_t syscall_get_screen_dimensions(uint64_t *width, uint64_t *height) {
    if (width == NULL || height == NULL) {
        return 0;
    }
    *width = video_get_width();
    *height = video_get_height();
    return 1;
}
uint64_t syscall_malloc(uint64_t size, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (size == 0) {
        return 0;
    }
    void *ptr = mm_alloc(size);
    return (uint64_t)ptr;
}

uint64_t syscall_free(uint64_t address, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (address == 0) {
        return 0;
    }
    mm_free((void *)address);
    return 1;
}

uint64_t syscall_meminfo(uint64_t user_addr, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (user_addr == 0) {
        return 0;
    }
    mm_stats_t stats;
    mm_get_stats(&stats);
    memcpy((void *)user_addr, &stats, sizeof(stats));
    return 1;
}

uint64_t syscall_create_process(char *name, void *function, char *argv[], uint64_t priority, uint64_t is_foreground_and_pipes) {
    // Desempaquetar parámetros:
    // Bits 0-31: stdin_pipe_id
    // Bits 32-63: stdout_pipe_id (solo si el bit más alto de is_foreground == 1, sino es is_foreground)
    // Si is_foreground_and_pipes <= 1, entonces es el viejo formato (solo is_foreground)
    
    int is_foreground = 0;
    uint64_t stdin_pipe_id = 0;
    uint64_t stdout_pipe_id = 0;
    
    if (is_foreground_and_pipes <= 1) {
        // Formato viejo: solo is_foreground
        is_foreground = (int)is_foreground_and_pipes;
    } else {
        // Formato nuevo: empaquetado
        // Bits 0-15: stdin_pipe_id
        // Bits 16-31: stdout_pipe_id  
        // Bit 32: is_foreground
        stdin_pipe_id = is_foreground_and_pipes & 0xFFFF;
        stdout_pipe_id = (is_foreground_and_pipes >> 16) & 0xFFFF;
        is_foreground = (is_foreground_and_pipes >> 32) & 0x1;
    }
    
    process_t *parent = scheduler_current_process();
    
    // Si el proceso hijo va a foreground, el padre debe perder foreground
    if (is_foreground && parent) {
        parent->is_foreground = 0;
    }
    
    process_t *p = scheduler_spawn_process(
        name ? name : "user_process",
        (process_entry_point_t)function,
        (void *)argv,
        parent,  // Pasar el padre para poder restaurar foreground después
        (uint8_t)priority,
        is_foreground,
        stdin_pipe_id,
        stdout_pipe_id
    );
    if (!p) {
        // Si falla, restaurar foreground del padre
        if (is_foreground && parent) {
            parent->is_foreground = 1;
        }
        return 0;
    }
    return p->pid;
}

uint64_t syscall_kill(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return scheduler_kill_by_pid(pid);
}

uint64_t syscall_block(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return scheduler_block_by_pid(pid);
}

uint64_t syscall_unblock(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return scheduler_unblock_by_pid(pid);
}

uint64_t syscall_get_type_of_mm(uint64_t user_addr, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (user_addr == 0) return 0;
    const char *name = mm_get_manager_name();
    size_t len = strlen(name) + 1;
    memcpy((void *)user_addr, name, len);
    return 1;
}

uint64_t syscall_getpid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    process_t *p = scheduler_current_process();
    return p ? p->pid : 0;
}

uint64_t syscall_set_priority(uint64_t pid, uint64_t newPrio, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return (uint64_t)scheduler_set_priority(pid, (uint8_t)newPrio);
}

uint64_t syscall_wait(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if ((int64_t)pid <= 0) return 0;
    process_t *target = scheduler_find_by_pid(pid);
    if (!target) return 0;

    if (target->state == PROCESS_STATE_FINISHED) {
        // Si el hijo estaba en foreground, restaurar foreground del padre
        process_t *me = scheduler_current_process();
        if (me && target->is_foreground) {
            me->is_foreground = 1;
        }
        return 1;
    }

    process_t *me = scheduler_current_process();
    // me == NULL es redundante porque ya se verificó !me
    if (!me || me == target) return 0;
    
    me->waiting_on_pid = pid;
    me->waiter_next = target->waiters_head;
    target->waiters_head = me;
    scheduler_block_current();
    scheduler_yield_current();
    
    // Cuando el wait termine, restaurar foreground si el hijo lo tenía
    if (target->is_foreground) {
        me->is_foreground = 1;
    }
    
    return 1;
}

uint64_t syscall_sem_create(int initial) {
    if (initial < 0) return 0;
    uint64_t id = sem_alloc(initial);
    return id;
}

uint64_t syscall_sem_open(uint64_t sem_id) {
    return sem_open_by_id(sem_id);
}

uint64_t syscall_sem_close(uint64_t sem_id) {
    return sem_close_by_id(sem_id);
}

uint64_t syscall_sem_wait(uint64_t sem_id) {
    return sem_wait_by_id(sem_id);
}

uint64_t syscall_sem_signal(uint64_t sem_id) {
    return sem_signal_by_id(sem_id);
}

uint64_t syscall_sem_set(uint64_t sem_id, int newval) {
    return sem_set_by_id(sem_id, newval);
}

uint64_t syscall_sem_get(uint64_t sem_id) {
    int val = 0;
    if (!sem_get_value_by_id(sem_id, &val)) return (uint64_t)-1;
    return (uint64_t)val;
}

uint64_t syscall_list_processes(uint64_t user_addr, uint64_t max_count, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (user_addr == 0 || max_count == 0 || max_count > MAX_PROCESS_INFO) {
        return 0;
    }
    uint64_t count = scheduler_list_all_processes((process_info_t *)user_addr, max_count);
    return count;
}

uint64_t syscall_yield(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    process_t *p = scheduler_current_process();
    if (!p) {
        return 0;
    }
    scheduler_yield_current();
    return 1;
}

// Syscalls de pipes
uint64_t syscall_pipe_create(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    return pipe_create();
}

uint64_t syscall_pipe_open(uint64_t pipe_id, uint64_t is_writer, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    return pipe_open_by_id(pipe_id, (int)is_writer);
}

uint64_t syscall_pipe_close(uint64_t pipe_id, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    if (pipe_id == 0) {
        return 0;
    }
    
    process_t *current_process = scheduler_current_process();
    if (!current_process) {
        return 0;
    }
    
    // Buscar el pipe en la tabla de FDs del proceso para determinar el tipo
    int is_writer = 0;  // Por defecto asumimos lector
    int found = 0;
    
    for (int i = 0; i < MAX_FDS; i++) {
        fd_entry_t *entry = &current_process->fds[i];
        if ((entry->type == FD_TYPE_PIPE_READ || entry->type == FD_TYPE_PIPE_WRITE) && entry->pipe) {
            uint64_t entry_pipe_id = pipe_get_id(entry->pipe);
            if (entry_pipe_id == pipe_id) {
                is_writer = (entry->type == FD_TYPE_PIPE_WRITE) ? 1 : 0;
                found = 1;
                break;
            }
        }
    }
    
    // Si no encontramos el pipe en la tabla de FDs, asumimos escritor por compatibilidad
    // (aunque esto no debería pasar en uso normal)
    if (!found) {
        is_writer = 1;
    }
    
    return pipe_close_by_id(pipe_id, is_writer);
}

// Asignar un pipe a un file descriptor específico
// pipe_id: ID del pipe a asignar
// fd: file descriptor donde asignar el pipe
// mode: 0 para lectura, 1 para escritura
uint64_t syscall_pipe_dup(uint64_t pipe_id, uint64_t fd, uint64_t mode, uint64_t unused1, uint64_t unused2) {
    // fd es uint64_t, por lo que fd < 0 siempre es falso
    if (fd >= MAX_FDS) {
        return 0;
    }
    
    process_t *current_process = scheduler_current_process();
    if (!current_process) {
        return 0;
    }
    
    // Obtener el pipe
    pipe_t *pipe = pipe_get_by_id(pipe_id);
    if (!pipe) {
        return 0;
    }
    
    fd_entry_t *entry = &current_process->fds[fd];

    // Liberar pipe previo asociado a este descriptor (si lo hubiera)
    if (entry->type == FD_TYPE_PIPE_READ || entry->type == FD_TYPE_PIPE_WRITE) {
        if (entry->pipe) {
            uint64_t old_id = pipe_get_id(entry->pipe);
            if (old_id != 0) {
                // Determinar si el pipe previo era escritor o lector
                int is_writer = (entry->type == FD_TYPE_PIPE_WRITE) ? 1 : 0;
                pipe_close_by_id(old_id, is_writer);
            }
        }
        entry->pipe = NULL;
    }

    // Asignar el pipe al FD y abrir con el modo correcto
    int is_writer_mode = 0;
    if (mode == 0) {
        // Modo lectura
        entry->type = FD_TYPE_PIPE_READ;
        is_writer_mode = 0;
    } else if (mode == 1) {
        // Modo escritura
        entry->type = FD_TYPE_PIPE_WRITE;
        is_writer_mode = 1;
    } else {
        // Modo inválido
        return 0;
    }
    
    // Abrir el pipe con el modo correcto
    if (!pipe_open_by_id(pipe_id, is_writer_mode)) {
        return 0;
    }
    
    entry->pipe = pipe;
    
    return 1;
}

uint64_t syscall_pipe_release_fd(uint64_t fd, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
    // fd es uint64_t, por lo que fd < 0 siempre es falso
    if (fd >= MAX_FDS) {
        return 0;
    }

    process_t *current_process = scheduler_current_process();
    if (!current_process) {
        return 0;
    }

    fd_entry_t *entry = &current_process->fds[fd];
    if (entry->type == FD_TYPE_PIPE_READ || entry->type == FD_TYPE_PIPE_WRITE) {
        if (entry->pipe) {
            uint64_t pipe_id = pipe_get_id(entry->pipe);
            if (pipe_id != 0) {
                // Determinar si es escritor o lector basado en el tipo de FD
                int is_writer = (entry->type == FD_TYPE_PIPE_WRITE) ? 1 : 0;
                pipe_close_by_id(pipe_id, is_writer);
            }
        }
    }
    entry->type = FD_TYPE_TERMINAL;
    entry->pipe = NULL;
    return 1;
}

uint64_t syscall_get_foreground_pid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    return scheduler_get_foreground_pid();
}
