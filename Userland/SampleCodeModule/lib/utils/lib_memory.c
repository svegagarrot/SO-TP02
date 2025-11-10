// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../../include/lib.h"
#include "../../include/syscall.h"
#include <stddef.h>

void *malloc(size_t size) {
	return (void *) sys_malloc(size);
}

void free(void *ptr) {
	if (ptr != NULL) {
		sys_free(ptr);
	}
}

int memory_info(memory_info_t *info) {
	if (info == NULL) {
		return 0;
	}
	return (int) sys_meminfo(info);
}

int get_type_of_mm(char *buf, int buflen) {
	if (!buf || buflen <= 0)
		return 0;
	return (int) sys_get_type_of_mm(buf, buflen);
}
