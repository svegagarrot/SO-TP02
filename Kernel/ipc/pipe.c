// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <pipe.h>
#include <process.h>
#include <semaphore.h>
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
	int readers;		 // Contador de lectores
	int writers;		 // Contador de escritores
	uint64_t sem_items;	 // Semáforo para items disponibles (inicialmente = 0)
	uint64_t sem_spaces; // Semáforo para espacios disponibles (inicialmente = PIPE_BUFFER_SIZE)
	uint64_t mutex;		 // Mutex para exclusión mutua
	int used;
};

static pipe_t pipes[MAX_PIPES];
static uint64_t next_pipe_id = 1;

void pipe_system_init(void) {
	for (int i = 0; i < MAX_PIPES; i++) {
		pipes[i].used = 0;
		pipes[i].id = 0;
		pipes[i].read_pos = 0;
		pipes[i].write_pos = 0;
		pipes[i].count = 0;
		pipes[i].readers = 0;
		pipes[i].writers = 0;
		pipes[i].sem_items = 0;
		pipes[i].sem_spaces = 0;
		pipes[i].mutex = 0;
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
	if (id == 0)
		return NULL;
	return find_pipe_by_id(id);
}

uint64_t pipe_get_id(pipe_t *p) {
	if (!p)
		return 0;
	return p->id;
}

uint64_t pipe_create(void) {
	for (int i = 0; i < MAX_PIPES; ++i) {
		if (!pipes[i].used) {
			pipe_t *p = &pipes[i];

			memset(p, 0, sizeof(pipe_t));

			p->sem_items = sem_alloc(0);
			if (p->sem_items == 0) {
				return 0;
			}

			p->sem_spaces = sem_alloc(PIPE_BUFFER_SIZE);
			if (p->sem_spaces == 0) {
				sem_close_by_id(p->sem_items);
				return 0;
			}

			p->mutex = sem_alloc(1);
			if (p->mutex == 0) {
				sem_close_by_id(p->sem_items);
				sem_close_by_id(p->sem_spaces);
				return 0;
			}

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
	return 0;
}

int pipe_open_by_id(uint64_t id, int is_writer) {
	pipe_t *p = pipe_get_by_id(id);
	if (!p)
		return 0;

	sem_wait_by_id(p->mutex);
	if (is_writer) {
		p->writers++;
	}
	else {
		p->readers++;
	}
	sem_signal_by_id(p->mutex);

	return 1;
}

int pipe_close_by_id(uint64_t id, int is_writer) {
	pipe_t *p = pipe_get_by_id(id);
	if (!p)
		return 0;

	sem_wait_by_id(p->mutex);

	int was_last_writer = 0;

	if (is_writer) {
		if (p->writers > 0) {
			p->writers--;
			was_last_writer = (p->writers == 0);
		}
	}
	else {
		if (p->readers > 0) {
			p->readers--;
		}
	}

	int should_destroy = (p->readers == 0 && p->writers == 0);

	if (should_destroy) {
		uint64_t items_id = p->sem_items;
		uint64_t spaces_id = p->sem_spaces;
		uint64_t mutex_id = p->mutex;

		p->used = 0;
		p->id = 0;
		p->sem_items = 0;
		p->sem_spaces = 0;
		p->mutex = 0;
		p->read_pos = 0;
		p->write_pos = 0;
		p->count = 0;

		sem_signal_by_id(mutex_id);

		sem_close_by_id(items_id);
		sem_close_by_id(spaces_id);
		sem_close_by_id(mutex_id);
	}
	else {
		if (was_last_writer) {
			int readers_to_wake = p->readers;
			if (readers_to_wake > 0) {
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
		if (!sem_wait_by_id(p->mutex)) {
			break;
		}

		if (p->count == 0 && p->writers == 0) {
			sem_signal_by_id(p->mutex);
			break;
		}

		if (p->count > 0) {
			buffer[bytes_read] = p->buffer[p->read_pos];
			p->read_pos = (p->read_pos + 1) % PIPE_BUFFER_SIZE;
			p->count--;
			bytes_read++;

			sem_signal_by_id(p->mutex);
			sem_signal_by_id(p->sem_spaces);
		}
		else {
			if (bytes_read > 0) {
				sem_signal_by_id(p->mutex);
				break;
			}

			sem_signal_by_id(p->mutex);

			if (!sem_wait_by_id(p->sem_items)) {
				break;
			}
		}
	}

	return (int) bytes_read;
}

int pipe_write(pipe_t *p, const char *buffer, size_t count) {
	if (!p || !buffer || count == 0) {
		return 0;
	}

	size_t bytes_written = 0;

	while (bytes_written < count) {
		if (!sem_wait_by_id(p->mutex)) {
			break;
		}

		if (p->readers == 0) {
			sem_signal_by_id(p->mutex);
			break;
		}

		if (p->count < PIPE_BUFFER_SIZE) {
			p->buffer[p->write_pos] = buffer[bytes_written];
			p->write_pos = (p->write_pos + 1) % PIPE_BUFFER_SIZE;
			p->count++;
			bytes_written++;

			sem_signal_by_id(p->mutex);
			sem_signal_by_id(p->sem_items);
		}
		else {
			sem_signal_by_id(p->mutex);

			if (!sem_wait_by_id(p->sem_spaces)) {
				break;
			}
		}
	}

	return (int) bytes_written;
}
