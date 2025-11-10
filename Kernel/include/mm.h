#ifndef MM_H
#define MM_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uint64_t total_bytes;
	uint64_t used_bytes;
	uint64_t free_bytes;
	uint64_t largest_free_block;
	uint64_t allocations;
	uint64_t frees;
	uint64_t failed_allocations;
} mm_stats_t;

void mm_init(void *heap_start, uint64_t heap_size);
void mm_init_default(void);
void *mm_alloc(uint64_t size);
void mm_free(void *ptr);
void mm_get_stats(mm_stats_t *stats);
uint8_t mm_is_initialized(void);
const char *mm_get_manager_name(void);

#endif
