// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <interrupts.h>
#include <keyboardDriver.h>
#include <mm.h>
#include <pipe.h>
#include <process.h>
#include <scheduler.h>
#include <semaphore.h>
#include <stddef.h>
#include <string.h>
#include <syscalls_lib.h>
#include <time.h>
#include <videoDriver.h>

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

	fd_entry_t *fd_entry = &current_process->fds[fd];

	if (fd_entry->type == FD_TYPE_TERMINAL) {
		if (fd != STDIN) {
			return 0;
		}

		if (!current_process->is_foreground) {
			scheduler_block_by_pid(current_process->pid);
			return 0;
		}

		uint64_t read = 0;
		while (read < (uint64_t) count) {
			char c = keyboard_read_getchar();
			if (c == 0) {
				keyboard_wait_for_char();
				continue;
			}

			if (c == 0x04) {
				break;
			}

			buffer[read++] = c;
			if (c == '\n')
				break;
		}
		return read;
	}
	else if (fd_entry->type == FD_TYPE_PIPE_READ) {
		if (!fd_entry->pipe) {
			return 0;
		}
		return pipe_read(fd_entry->pipe, buffer, count);
	}
	else {
		return 0;
	}
}

uint64_t syscall_write(int fd, const char *buffer, int count) {
	if (buffer == NULL || count <= 0 || fd < 0 || fd >= MAX_FDS) {
		return 0;
	}

	process_t *current_process = scheduler_current_process();
	if (!current_process) {
		return 0;
	}

	fd_entry_t *fd_entry = &current_process->fds[fd];

	if (fd_entry->type == FD_TYPE_TERMINAL) {
		if (fd != STDOUT) {
			return 0;
		}

		for (int i = 0; i < count; i++) {
			video_putChar(buffer[i], FOREGROUND_COLOR, BACKGROUND_COLOR);
		}

		return count;
	}
	else if (fd_entry->type == FD_TYPE_PIPE_WRITE) {
		if (!fd_entry->pipe) {
			return 0;
		}
		return pipe_write(fd_entry->pipe, buffer, count);
	}
	else {
		return 0;
	}
}

uint64_t syscall_clearScreen() {
	video_clearScreen();
	return 1;
}

uint64_t syscall_sleep(int duration) {
	const uint32_t HZ = 18;
	uint64_t start = ticks_elapsed();
	uint64_t wait_tics = (duration * HZ + 999) / 1000;
	uint64_t target = start + wait_tics;

	while (ticks_elapsed() < target) {
		_hlt();
	}
	return 0;
}

uint64_t syscall_video_putChar(uint64_t c, uint64_t fg, uint64_t bg, uint64_t unused1, uint64_t unused2) {
	video_putChar((char) c, fg, bg);
	return 0;
}

uint64_t syscall_shutdown(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
	outw(0x604, 0x2000);

	_cli();

	while (1) {
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
	return (uint64_t) ptr;
}

uint64_t syscall_free(uint64_t address, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
	if (address == 0) {
		return 0;
	}
	mm_free((void *) address);
	return 1;
}

uint64_t syscall_meminfo(uint64_t user_addr, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
	if (user_addr == 0) {
		return 0;
	}
	mm_stats_t stats;
	mm_get_stats(&stats);
	memcpy((void *) user_addr, &stats, sizeof(stats));
	return 1;
}

uint64_t syscall_create_process(char *name, void *function, char *argv[], uint64_t priority,
								uint64_t is_foreground_and_pipes) {
	int is_foreground = 0;
	uint64_t stdin_pipe_id = 0;
	uint64_t stdout_pipe_id = 0;

	if (is_foreground_and_pipes <= 1) {
		is_foreground = (int) is_foreground_and_pipes;
	}
	else {
		stdin_pipe_id = is_foreground_and_pipes & 0xFFFF;
		stdout_pipe_id = (is_foreground_and_pipes >> 16) & 0xFFFF;
		is_foreground = (is_foreground_and_pipes >> 32) & 0x1;
	}

	process_t *parent = scheduler_current_process();

	if (is_foreground && parent) {
		parent->is_foreground = 0;
	}

	process_t *p =
		scheduler_spawn_process(name ? name : "user_process", (process_entry_point_t) function, (void *) argv,
								parent,
								(uint8_t) priority, is_foreground, stdin_pipe_id, stdout_pipe_id);
	if (!p) {
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

uint64_t syscall_get_type_of_mm(uint64_t user_addr, uint64_t unused1, uint64_t unused2, uint64_t unused3,
								uint64_t unused4) {
	if (user_addr == 0)
		return 0;
	const char *name = mm_get_manager_name();
	size_t len = strlen(name) + 1;
	memcpy((void *) user_addr, name, len);
	return 1;
}

uint64_t syscall_getpid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
	process_t *p = scheduler_current_process();
	return p ? p->pid : 0;
}

uint64_t syscall_set_priority(uint64_t pid, uint64_t newPrio, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
	return (uint64_t) scheduler_set_priority(pid, (uint8_t) newPrio);
}

uint64_t syscall_wait(uint64_t pid, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
	if ((int64_t) pid <= 0)
		return 0;
	process_t *target = scheduler_find_by_pid(pid);
	if (!target)
		return 0;

	if (target->state == PROCESS_STATE_FINISHED) {
		process_t *me = scheduler_current_process();
		if (me && target->is_foreground) {
			me->is_foreground = 1;
		}
		return 1;
	}

	process_t *me = scheduler_current_process();
	if (!me || me == target)
		return 0;

	me->waiting_on_pid = pid;
	me->waiter_next = target->waiters_head;
	target->waiters_head = me;
	scheduler_block_current();
	scheduler_yield_current();

	if (target->is_foreground) {
		me->is_foreground = 1;
	}

	return 1;
}

uint64_t syscall_sem_create(int initial) {
	if (initial < 0)
		return 0;
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
	if (!sem_get_value_by_id(sem_id, &val))
		return (uint64_t) -1;
	return (uint64_t) val;
}

uint64_t syscall_sem_set_random(uint64_t sem_id, uint64_t enable) {
	return sem_set_random_dequeue(sem_id, (int) enable);
}

uint64_t syscall_list_processes(uint64_t user_addr, uint64_t max_count, uint64_t unused2, uint64_t unused3,
								uint64_t unused4) {
	if (user_addr == 0 || max_count == 0 || max_count > MAX_PROCESS_INFO) {
		return 0;
	}
	uint64_t count = scheduler_list_all_processes((process_info_t *) user_addr, max_count);
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

uint64_t syscall_pipe_create(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
	return pipe_create();
}

uint64_t syscall_pipe_open(uint64_t pipe_id, uint64_t is_writer, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
	return pipe_open_by_id(pipe_id, (int) is_writer);
}

uint64_t syscall_pipe_close(uint64_t pipe_id, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
	if (pipe_id == 0) {
		return 0;
	}

	process_t *current_process = scheduler_current_process();
	if (!current_process) {
		return 0;
	}

	int is_writer = 0;
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

	if (!found) {
		is_writer = 1;
	}

	return pipe_close_by_id(pipe_id, is_writer);
}

uint64_t syscall_pipe_dup(uint64_t pipe_id, uint64_t fd, uint64_t mode, uint64_t unused1, uint64_t unused2) {
	if (fd >= MAX_FDS) {
		return 0;
	}

	process_t *current_process = scheduler_current_process();
	if (!current_process) {
		return 0;
	}

	pipe_t *pipe = pipe_get_by_id(pipe_id);
	if (!pipe) {
		return 0;
	}

	fd_entry_t *entry = &current_process->fds[fd];

	if (entry->type == FD_TYPE_PIPE_READ || entry->type == FD_TYPE_PIPE_WRITE) {
		if (entry->pipe) {
			uint64_t old_id = pipe_get_id(entry->pipe);
			if (old_id != 0) {
				int is_writer = (entry->type == FD_TYPE_PIPE_WRITE) ? 1 : 0;
				pipe_close_by_id(old_id, is_writer);
			}
		}
		entry->pipe = NULL;
	}

	int is_writer_mode = 0;
	if (mode == 0) {
		entry->type = FD_TYPE_PIPE_READ;
		is_writer_mode = 0;
	}
	else if (mode == 1) {
		entry->type = FD_TYPE_PIPE_WRITE;
		is_writer_mode = 1;
	}
	else {
		return 0;
	}

	if (!pipe_open_by_id(pipe_id, is_writer_mode)) {
		return 0;
	}

	entry->pipe = pipe;

	return 1;
}

uint64_t syscall_pipe_release_fd(uint64_t fd, uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4) {
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
				int is_writer = (entry->type == FD_TYPE_PIPE_WRITE) ? 1 : 0;
				pipe_close_by_id(pipe_id, is_writer);
			}
		}
	}
	entry->type = FD_TYPE_TERMINAL;
	entry->pipe = NULL;
	return 1;
}

uint64_t syscall_get_foreground_pid(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4,
									uint64_t unused5) {
	return scheduler_get_foreground_pid();
}
