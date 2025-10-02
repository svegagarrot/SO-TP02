
#include "syscall.h"
#include "test_util.h"
#include "../include/lib.h"
//#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BLOCKS 128

typedef struct MM_rq {
  void *address;
  uint32_t size;
} mm_rq;

uint64_t test_mm(uint64_t argc, char *argv[]) {

  mm_rq mm_rqs[MAX_BLOCKS];
  uint8_t rq;
  uint32_t total;
  uint64_t max_memory;

  if (argc != 1)
    return -1;

  if ((max_memory = satoi(argv[0])) <= 0)
    return -1;

  printf("=== TEST DE MEMORIA INICIADO ===\n");
  printf("Memoria maxima: %d bytes\n", max_memory);
  printf("Bloques maximos: %d\n", MAX_BLOCKS);
  printf("Presiona Ctrl+C para detener\n\n");

  uint64_t iteration = 0;
  while (1) {
    iteration++;
    rq = 0;
    total = 0;

    printf("Iteracion %d: ", iteration);

    // Request as many blocks as we can
    while (rq < MAX_BLOCKS && total < max_memory) {
      mm_rqs[rq].size = GetUniform(max_memory - total - 1) + 1;
      mm_rqs[rq].address = malloc(mm_rqs[rq].size);

      if (mm_rqs[rq].address) {
        total += mm_rqs[rq].size;
        rq++;
      } else {
        // Si malloc falla, salimos del bucle
        break;
      }
    }

    printf("Asignados %d bloques (%d bytes) - ", rq, total);

    // Set
    uint32_t i;
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        memset(mm_rqs[i].address, i, mm_rqs[i].size);

    printf("Escritura OK - ");

    // Check
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        if (!memcheck(mm_rqs[i].address, i, mm_rqs[i].size)) {
          printf("ERROR en verificacion!\n");
          return -1;
        }

    printf("Verificacion OK - ");

    // Free
    for (i = 0; i < rq; i++)
      if (mm_rqs[i].address)
        free(mm_rqs[i].address);

    printf("Liberacion OK\n");
  }
}