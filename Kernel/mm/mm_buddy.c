#include "mm.h"
#include <stdint.h>
#include <stddef.h>
#include <lib.h>
#include <stdio.h>

#ifdef USE_BUDDY_MM
#ifndef MM_DEFAULT_HEAP_LIMIT
#define MM_DEFAULT_HEAP_LIMIT   0x20000000ULL
#endif

// --- Constantes internas / helpers de alineación ---
#define MIN_ALIGN               16ULL
static inline uint64_t align_up_u64(uint64_t v, uint64_t a) { return (v + a - 1) & ~(a - 1); }
static inline uint64_t pow2_u64(uint8_t e)                  { return 1ULL << e; }

// --- Estructura de bloque del buddy ---
typedef struct buddy_block {
    struct buddy_block *next;
    struct buddy_block *prev;
    uint8_t level;     // tamaño del bloque = (2^level) * MM_MIN_BLOCK_SIZE
    uint8_t is_free;   // 1 = libre, 0 = tomado
} buddy_block_t;

// Tamaños derivados (HDR_SIZE debe ser múltiplo de 16)
#define HDR_SIZE                align_up_u64(sizeof(buddy_block_t), MIN_ALIGN)
// Bloque mínimo: header + al menos 16B de payload, 16-aligned
#define MM_MIN_BLOCK_SIZE       (HDR_SIZE + 16ULL)
#define MM_MAX_LEVELS           32

// --- Listas de libres por nivel ---
static buddy_block_t *free_lists[MM_MAX_LEVELS];

// --- Estado global / estadísticas (misma semántica que mm_simple) ---
static uint8_t  mm_initialized_flag   = 0;
static uint64_t heap_capacity_bytes   = 0;   // capacidad de payload (stats)
static uint64_t heap_used_bytes       = 0;
static uint64_t heap_allocation_count = 0;
static uint64_t heap_free_count       = 0;
static uint64_t heap_failed_allocations = 0;

// --- Región real del heap (para validaciones y buddy math) ---
static void    *heap_base         = NULL;
static uint64_t heap_usable_bytes = 0;  // bytes usables totales (incluye headers)
static uint8_t  max_level         = 0;

// ----------------------- Helpers internos -----------------------
static void free_lists_init(void) {
    for (int i = 0; i < MM_MAX_LEVELS; i++) free_lists[i] = NULL;
}

static uint8_t order_for(uint64_t size_bytes) {
    // tamaño total que necesito: payload+header, redondeado a bloque mínimo
    uint64_t total = (size_bytes < MM_MIN_BLOCK_SIZE) ? MM_MIN_BLOCK_SIZE : size_bytes;
    uint8_t order = 0;
    uint64_t blk = MM_MIN_BLOCK_SIZE;
    while (blk < total && order < (MM_MAX_LEVELS - 1)) {
        blk <<= 1;
        order++;
    }
    return order;
}

static void push_free(buddy_block_t *b) {
    b->is_free = 1;
    b->prev = NULL;
    b->next = free_lists[b->level];
    if (b->next) b->next->prev = b;
    free_lists[b->level] = b;
}

static buddy_block_t* pop_free(uint8_t level) {
    buddy_block_t *b = free_lists[level];
    if (!b) return NULL;
    free_lists[level] = b->next;
    if (free_lists[level]) free_lists[level]->prev = NULL;
    b->next = b->prev = NULL;
    b->is_free = 0;
    return b;
}

static buddy_block_t* split_down(uint8_t from_level, uint8_t to_level) {
    // Parte sucesivamente un bloque grande hasta alcanzar el nivel deseado
    for (uint8_t l = from_level; l > to_level; l--) {
        buddy_block_t *blk = pop_free(l);
        if (!blk) return NULL;
        uint64_t size = pow2_u64(l) * MM_MIN_BLOCK_SIZE;

        // segunda mitad: buddy
        buddy_block_t *buddy = (buddy_block_t *)((uint8_t *)blk + (size >> 1));
        buddy->level   = l - 1;
        buddy->is_free = 1;
        buddy->next = buddy->prev = NULL;
        push_free(buddy);

        // primera mitad: el mismo header (blk) baja un nivel
        blk->level   = l - 1;
        blk->is_free = 1;
        blk->next = blk->prev = NULL;
        push_free(blk);
    }
    return pop_free(to_level);
}

static buddy_block_t* get_block_from_ptr(void *ptr) {
    if (!ptr) return NULL;
    return (buddy_block_t *)((uint8_t *)ptr - HDR_SIZE);
}

static buddy_block_t* find_buddy(buddy_block_t *blk) {
    // Calcular offset relativo, tamaño de bloque y XOR
    uint64_t offset     = (uint64_t)((uint8_t*)blk - (uint8_t*)heap_base);
    uint64_t block_size = pow2_u64(blk->level) * MM_MIN_BLOCK_SIZE;
    uint64_t buddy_off  = offset ^ block_size;

    // Validación de límites con el tamaño usable real del heap
    if (buddy_off + block_size > heap_usable_bytes) return NULL;

    return (buddy_block_t *)((uint8_t*)heap_base + buddy_off);
}

static void remove_from_free_list(buddy_block_t *b) {
    buddy_block_t **head = &free_lists[b->level];
    while (*head && *head != b) head = &((*head)->next);
    if (*head == b) {
        *head = b->next;
        if (b->next) b->next->prev = b->prev;
        b->next = b->prev = NULL;
    }
}

static void coalesce(buddy_block_t *blk) {
    // Subir combinando mientras el buddy esté libre y al mismo nivel
    while (blk->level < max_level) {
        buddy_block_t *bud = find_buddy(blk);
        if (!bud || !bud->is_free || bud->level != blk->level) break;

        // Sacar buddy de su free list
        remove_from_free_list(bud);

        // Elegir el que queda "más a la izquierda" como header del bloque combinado
        if (bud < blk) blk = bud;

        blk->level++;
        blk->is_free = 1;
    }
    push_free(blk);
}

// ------------------- API pública (mm.h) -------------------
void mm_init(void *heap_start, uint64_t heap_size) {
    // Validaciones básicas
    if (heap_size < MM_MIN_BLOCK_SIZE) {
        mm_initialized_flag = 0;
        heap_capacity_bytes = 0;
        heap_usable_bytes   = 0;
        return;
    }

    // Alinear base y tamaño
    uint64_t aligned_start  = align_up_u64((uint64_t)heap_start, MIN_ALIGN);
    uint64_t alignment_loss = aligned_start - (uint64_t)heap_start;
    if (alignment_loss >= heap_size) {
        mm_initialized_flag = 0;
        heap_capacity_bytes = 0;
        heap_usable_bytes   = 0;
        return;
    }

    uint64_t usable = heap_size - alignment_loss;
    usable &= ~(MIN_ALIGN - 1ULL);
    if (usable < MM_MIN_BLOCK_SIZE) {
        mm_initialized_flag = 0;
        heap_capacity_bytes = 0;
        heap_usable_bytes   = 0;
        return;
    }

    heap_base         = (void*)aligned_start;
    heap_usable_bytes = usable;

    // Calcular nivel máximo (bloque más grande que entra)
    max_level = 0;
    uint64_t sz = MM_MIN_BLOCK_SIZE;
    while (sz < usable && max_level < (MM_MAX_LEVELS - 1)) {
        sz <<= 1;
        max_level++;
    }

    // Inicializar free lists
    free_lists_init();

    // Publicar TODA la región mediante una descomposición voraz
    uint8_t  level     = max_level;
    uint64_t remaining = usable;
    uint8_t *cur       = (uint8_t*)heap_base;

    while (remaining >= MM_MIN_BLOCK_SIZE && level < MM_MAX_LEVELS) {
        uint64_t blk_size = pow2_u64(level) * MM_MIN_BLOCK_SIZE;
        if (blk_size > remaining) { level--; continue; }

        buddy_block_t *b = (buddy_block_t*)cur;
        b->next = b->prev = NULL;
        b->level = level;
        b->is_free = 1;
        push_free(b);

        cur       += blk_size;
        remaining -= blk_size;
    }

    // Estadísticas iniciales (misma semántica que mm_simple)
    heap_capacity_bytes     = usable - HDR_SIZE; // capacidad de payload (aprox)
    heap_used_bytes         = 0;
    heap_allocation_count   = 0;
    heap_free_count         = 0;
    heap_failed_allocations = 0;

    mm_initialized_flag = 1;
}

static void mm_default_region(void **start, uint64_t *size) {
    extern uint8_t endOfKernel;
    const uint64_t MM_PAGE_SIZE          = 0x1000ULL;
    const uint64_t MM_KERNEL_STACK_PAGES = 8ULL;

    const uint64_t raw_start     = (uint64_t)&endOfKernel + MM_PAGE_SIZE * MM_KERNEL_STACK_PAGES;
    const uint64_t aligned_start = align_up_u64(raw_start, MIN_ALIGN);
    const uint64_t heap_limit    = MM_DEFAULT_HEAP_LIMIT;

    if (aligned_start >= heap_limit) {
        *start = NULL; *size = 0; return;
    }
    *start = (void*)aligned_start;
    *size  = heap_limit - aligned_start;
}

void mm_init_default(void) {
    void *start = NULL; uint64_t size = 0;
    mm_default_region(&start, &size);
    if (start && size) mm_init(start, size);
}

void* mm_alloc(uint64_t size) {
    if (!mm_initialized_flag) {
        mm_init_default();
        if (!mm_initialized_flag) { heap_failed_allocations++; return NULL; }
    }
    if (size == 0) return NULL;

    uint64_t req = align_up_u64(size, MIN_ALIGN);
    uint8_t  ord = order_for(req + HDR_SIZE);

    buddy_block_t *blk = pop_free(ord);
    if (!blk) {
        // buscar un bloque más grande y partirlo
        for (uint8_t l = ord + 1; l <= max_level; l++) {
            if (free_lists[l]) { blk = split_down(l, ord); break; }
        }
    }
    if (!blk) { heap_failed_allocations++; return NULL; }

    blk->is_free = 0;

    // Stats (payload estimada = tamaño bloque - header)
    uint64_t payload = pow2_u64(ord) * MM_MIN_BLOCK_SIZE - HDR_SIZE;
    heap_used_bytes += payload;
    if (heap_used_bytes > heap_capacity_bytes) heap_used_bytes = heap_capacity_bytes;

    return (uint8_t*)blk + HDR_SIZE;
}

void mm_free(void *ptr) {
    if (!ptr || !mm_initialized_flag) return;

    buddy_block_t *blk = get_block_from_ptr(ptr);
    if (!blk || blk->is_free) return; // defensa simple ante double free

    blk->is_free = 1;
    heap_free_count++;

    uint64_t payload = pow2_u64(blk->level) * MM_MIN_BLOCK_SIZE - HDR_SIZE;
    if (heap_used_bytes >= payload) heap_used_bytes -= payload;
    else heap_used_bytes = 0;

    coalesce(blk);
}

void mm_get_stats(mm_stats_t *s) {
    if (!s) return;

    uint64_t largest = 0;
    for (uint8_t l = 0; l <= max_level; l++) {
        for (buddy_block_t *cur = free_lists[l]; cur; cur = cur->next) {
            uint64_t cur_payload = pow2_u64(cur->level) * MM_MIN_BLOCK_SIZE - HDR_SIZE;
            if (cur_payload > largest) largest = cur_payload;
        }
    }

    s->total_bytes        = heap_capacity_bytes;
    s->used_bytes         = heap_used_bytes;
    s->free_bytes         = (heap_capacity_bytes > heap_used_bytes) ? (heap_capacity_bytes - heap_used_bytes) : 0;
    s->largest_free_block = largest;
    s->allocations        = heap_allocation_count;
    s->frees              = heap_free_count;
    s->failed_allocations = heap_failed_allocations;
}

uint8_t mm_is_initialized(void) { return mm_initialized_flag; }

const char *mm_get_manager_name(void) { return "buddy"; }

#endif // USE_BUDDY_MM
