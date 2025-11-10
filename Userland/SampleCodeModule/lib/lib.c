// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/lib.h"
#include "../include/syscall.h"
#include <stddef.h>

// ============================================================================
// MISCELLANEOUS UTILITY FUNCTIONS
// ============================================================================

int try_getchar(char *c) {
	(void) c;
	return 0;
}

void clearScreen() {
	sys_clearScreen();
}

void sleep(int ms) {
	sys_sleep(ms);
}

void clear_key_buffer() {
}

int getScreenDims(uint64_t *width, uint64_t *height) {
	return sys_screenDims(width, height);
}

void shutdown() {
	sys_shutdown();
}

// ============================================================================
// PIPE FUNCTIONS
// ============================================================================

uint64_t pipe_create(void) {
	return sys_pipe_create();
}

uint64_t pipe_open(uint64_t pipe_id) {
	if (pipe_id == 0) {
		return 0;
	}
	return sys_pipe_open(pipe_id);
}

uint64_t pipe_close(uint64_t pipe_id) {
	if (pipe_id == 0) {
		return 0;
	}
	return sys_pipe_close(pipe_id);
}

uint64_t pipe_dup(uint64_t pipe_id, uint64_t fd, uint64_t mode) {
	if (pipe_id == 0) {
		return 0;
	}
	if (mode != 0 && mode != 1) {
		return 0; // mode debe ser 0 (lectura) o 1 (escritura)
	}
	return sys_pipe_dup(pipe_id, fd, mode);
}

uint64_t pipe_release_fd(uint64_t fd) {
	return sys_pipe_release_fd(fd);
}

uint64_t get_foreground_pid(void) {
	return sys_get_foreground_pid();
}
