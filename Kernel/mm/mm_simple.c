// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <lib.h>
#include <mm.h>
#include <stdint.h>

#ifdef USE_SIMPLE_MM

#ifndef MM_DEFAULT_HEAP_LIMIT
#define MM_DEFAULT_HEAP_LIMIT 0x20000000ULL
#endif

#define MM_PAGE_SIZE 0x1000ULL
#define MM_KERNEL_STACK_PAGES 8ULL
#define MM_ALIGNMENT 16ULL

#define BLOCK_ALLOCATED_FLAG 1ULL

typedef struct block_header block_header_t;

struct block_header {
	uint64_t size_and_flags;   /* block size including header; LSB used as allocated flag */
	block_header_t *prev_phys; /* previous block in physical order */
	block_header_t *next_phys; /* next block in physical order */
	block_header_t *prev_free; /* previous block in free list */
	block_header_t *next_free; /* next block in free list */
};

static block_header_t free_list_sentinel;
static uint8_t mm_initialized_flag = 0;
static uint64_t heap_capacity_bytes = 0;
static uint64_t heap_used_bytes = 0;
static uint64_t heap_allocation_count = 0;
static uint64_t heap_free_count = 0;
static uint64_t heap_failed_allocations = 0;

static inline uint64_t align_up(uint64_t value, uint64_t alignment) {
	uint64_t result = (value + alignment - 1) & ~(alignment - 1);
	return result;
}

static inline uint64_t block_size(const block_header_t *block) {
	uint64_t size = block->size_and_flags & ~BLOCK_ALLOCATED_FLAG;
	return size;
}

static inline int block_is_allocated(const block_header_t *block) {
	int allocated = (block->size_and_flags & BLOCK_ALLOCATED_FLAG) != 0;
	return allocated;
}

static inline void block_mark_allocated(block_header_t *block) {
	block->size_and_flags |= BLOCK_ALLOCATED_FLAG;
}

static inline void block_mark_free(block_header_t *block) {
	block->size_and_flags &= ~BLOCK_ALLOCATED_FLAG;
}

static void free_list_init(void) {
	free_list_sentinel.prev_free = &free_list_sentinel;
	free_list_sentinel.next_free = &free_list_sentinel;
}

static void free_list_insert(block_header_t *block) {
	block->prev_free = &free_list_sentinel;
	block->next_free = free_list_sentinel.next_free;
	free_list_sentinel.next_free->prev_free = block;
	free_list_sentinel.next_free = block;
}

static void free_list_remove(block_header_t *block) {
	block->prev_free->next_free = block->next_free;
	block->next_free->prev_free = block->prev_free;
	block->prev_free = NULL;
	block->next_free = NULL;
}

static block_header_t *find_first_fit(uint64_t required_size) {
	for (block_header_t *current = free_list_sentinel.next_free; current != &free_list_sentinel;
		 current = current->next_free) {
		uint64_t current_size = block_size(current);
		if (current_size >= required_size) {
			return current;
		}
	}
	return NULL;
}

static block_header_t *split_block_if_possible(block_header_t *block, uint64_t required_size) {
	const uint64_t total_size = block_size(block);
	const uint64_t header_size = sizeof(block_header_t);
	const uint64_t min_block_size = header_size + MM_ALIGNMENT;

	if (total_size < required_size + min_block_size) {
		return block;
	}

	uint64_t remaining_size = total_size - required_size;
	block_header_t *new_block = (block_header_t *) ((uint8_t *) block + required_size);

	new_block->size_and_flags = remaining_size;
	new_block->prev_phys = block;
	new_block->next_phys = block->next_phys;
	if (new_block->next_phys != NULL) {
		new_block->next_phys->prev_phys = new_block;
	}
	block->next_phys = new_block;

	block->size_and_flags = required_size | (block->size_and_flags & BLOCK_ALLOCATED_FLAG);

	block_mark_free(new_block);
	new_block->prev_free = NULL;
	new_block->next_free = NULL;
	free_list_insert(new_block);

	return block;
}

static block_header_t *coalesce_with_neighbors(block_header_t *block) {
	block_header_t *prev = block->prev_phys;
	block_header_t *next = block->next_phys;

	if (prev != NULL && !block_is_allocated(prev)) {
		free_list_remove(prev);
		prev->size_and_flags = block_size(prev) + block_size(block);
		prev->next_phys = block->next_phys;
		if (block->next_phys != NULL) {
			block->next_phys->prev_phys = prev;
		}
		block = prev;
	}

	if (next != NULL && !block_is_allocated(next)) {
		free_list_remove(next);
		block->size_and_flags = block_size(block) + block_size(next);
		block->next_phys = next->next_phys;
		if (next->next_phys != NULL) {
			next->next_phys->prev_phys = block;
		}
	}

	return block;
}

void mm_init(void *heap_start, uint64_t heap_size) {
	const uint64_t min_block_size = sizeof(block_header_t) + MM_ALIGNMENT;

	if (heap_size < min_block_size) {
		mm_initialized_flag = 0;
		heap_capacity_bytes = 0;
		heap_used_bytes = 0;
		heap_allocation_count = 0;
		heap_free_count = 0;
		heap_failed_allocations = 0;
		return;
	}

	uint64_t aligned_start = align_up((uint64_t) heap_start, MM_ALIGNMENT);
	if (aligned_start < (uint64_t) heap_start) {
		mm_initialized_flag = 0;
		return;
	}

	uint64_t alignment_loss = aligned_start - (uint64_t) heap_start;
	if (alignment_loss >= heap_size) {
		mm_initialized_flag = 0;
		return;
	}

	uint64_t usable_bytes = heap_size - alignment_loss;
	usable_bytes &= ~(MM_ALIGNMENT - 1);

	if (usable_bytes < min_block_size) {
		mm_initialized_flag = 0;
		heap_capacity_bytes = 0;
		heap_used_bytes = 0;
		heap_allocation_count = 0;
		heap_free_count = 0;
		heap_failed_allocations = 0;
		return;
	}

	free_list_init();

	block_header_t *initial_block = (block_header_t *) aligned_start;
	initial_block->size_and_flags = usable_bytes;
	initial_block->prev_phys = NULL;
	initial_block->next_phys = NULL;
	block_mark_free(initial_block);

	free_list_insert(initial_block);

	heap_capacity_bytes = usable_bytes - sizeof(block_header_t);
	heap_used_bytes = 0;
	heap_allocation_count = 0;
	heap_free_count = 0;
	heap_failed_allocations = 0;
	mm_initialized_flag = 1;
}

static void mm_default_region(void **start, uint64_t *size) {
	extern uint8_t endOfKernel;
	const uint64_t raw_start = (uint64_t) &endOfKernel + MM_PAGE_SIZE * MM_KERNEL_STACK_PAGES;
	const uint64_t aligned_start = align_up(raw_start, MM_ALIGNMENT);
	const uint64_t heap_limit = MM_DEFAULT_HEAP_LIMIT;

	if (aligned_start >= heap_limit) {
		*start = NULL;
		*size = 0;
		return;
	}

	*start = (void *) aligned_start;
	*size = heap_limit - aligned_start;
}

void mm_init_default(void) {
	void *start = NULL;
	uint64_t size = 0;
	mm_default_region(&start, &size);
	if (start != NULL && size > 0) {
		mm_init(start, size);
	}
}

void *mm_alloc(uint64_t size) {
	if (!mm_initialized_flag) {
		mm_init_default();
		if (!mm_initialized_flag) {
			heap_failed_allocations++;
			return NULL;
		}
	}

	if (size == 0) {
		return NULL;
	}

	const uint64_t header_size = sizeof(block_header_t);
	const uint64_t min_block_size = header_size + MM_ALIGNMENT;

	uint64_t aligned_size = align_up(size, MM_ALIGNMENT);
	uint64_t required_size = aligned_size + header_size;
	if (required_size < min_block_size) {
		required_size = min_block_size;
	}

	block_header_t *block = find_first_fit(required_size);
	if (block == NULL) {
		heap_failed_allocations++;
		return NULL;
	}
	free_list_remove(block);
	block = split_block_if_possible(block, required_size);
	block_mark_allocated(block);

	uint64_t payload_size = block_size(block) - header_size;
	heap_used_bytes += payload_size;
	if (heap_used_bytes > heap_capacity_bytes) {
		heap_used_bytes = heap_capacity_bytes;
	}
	heap_allocation_count++;
	void *result = (uint8_t *) block + header_size;
	return result;
}

void mm_free(void *ptr) {
	if (ptr == NULL || !mm_initialized_flag) {
		return;
	}

	block_header_t *block = (block_header_t *) ((uint8_t *) ptr - sizeof(block_header_t));
	if (!block_is_allocated(block)) {
		return;
	}
	block_mark_free(block);
	heap_free_count++;

	uint64_t payload_size = block_size(block) - sizeof(block_header_t);
	if (heap_used_bytes >= payload_size) {
		heap_used_bytes -= payload_size;
	}
	else {
		heap_used_bytes = 0;
	}

	block->prev_free = NULL;
	block->next_free = NULL;

	block = coalesce_with_neighbors(block);
	free_list_insert(block);
}

void mm_get_stats(mm_stats_t *stats) {
	if (stats == NULL) {
		return;
	}

	uint64_t largest_block = 0;
	for (block_header_t *current = free_list_sentinel.next_free; current != &free_list_sentinel;
		 current = current->next_free) {
		uint64_t current_size = block_size(current);
		if (current_size > largest_block) {
			largest_block = current_size;
		}
	}

	if (largest_block >= sizeof(block_header_t)) {
		largest_block -= sizeof(block_header_t);
	}
	else {
		largest_block = 0;
	}

	stats->total_bytes = heap_capacity_bytes;
	stats->used_bytes = heap_used_bytes;
	stats->free_bytes = (heap_capacity_bytes > heap_used_bytes) ? (heap_capacity_bytes - heap_used_bytes) : 0;
	stats->largest_free_block = largest_block;
	stats->allocations = heap_allocation_count;
	stats->frees = heap_free_count;
	stats->failed_allocations = heap_failed_allocations;
}

uint8_t mm_is_initialized(void) {
	uint8_t initialized = mm_initialized_flag;
	return initialized;
}

const char *mm_get_manager_name(void) {
	return "simple";
}

#endif /* USE_SIMPLE_MM */
