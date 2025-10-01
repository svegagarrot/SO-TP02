#include "time.h"
#include "scheduler.h"
#include <stdint.h>

#define QUANTUM_TICKS 18  // 1 segundo (18 ticks = 1 segundo)
#define MAX_PROCESSES 10

extern void _hlt();

typedef struct {
    uint64_t stack_pointer;
    uint64_t pid;
    uint8_t state;  // 0=ready, 1=running, 2=blocked
} process_t;

static process_t processes[MAX_PROCESSES];
static uint8_t current_pid = 0;
static uint8_t process_count = 0;
static uint64_t last_switch_tick = 0;

static uint64_t idle_stack[1024];  // Stack para el proceso idle
static uint8_t idle_pid = 0xFF;    // PID especial para idle

void init_scheduler() {
    // Crear stack para idle con frame válido
    uint64_t idle_rsp = (uint64_t)&idle_stack[1023];
    
    // Simular un stack frame para idle_process
    idle_stack[1023] = (uint64_t)idle_process;  // RIP
    idle_stack[1022] = 0;                       // RFLAGS
    idle_stack[1021] = idle_rsp;                // RSP
    
    processes[0].stack_pointer = idle_rsp - 24; // Ajustar para el frame
    processes[0].pid = idle_pid;
    processes[0].state = 1;
    process_count = 1;
    current_pid = 0;
}


// Función que llama el timer interrupt
uint64_t schedule(uint64_t current_rsp) {
    uint64_t current_tick = ticks_elapsed();
    
    // Solo hacer context switch cada QUANTUM_TICKS
    if (current_tick - last_switch_tick < QUANTUM_TICKS) {
        return current_rsp;  // No hay switch, devolver mismo SP
    }
    
    // Guardar contexto del proceso actual
    processes[current_pid].stack_pointer = current_rsp;
    
    // Buscar siguiente proceso listo
    uint8_t next_pid = (current_pid + 1) % process_count;
    while (processes[next_pid].state != 0 && next_pid != current_pid) {
        next_pid = (next_pid + 1) % process_count;
    }
    
    // Si no hay procesos listos, usar idle
    if (processes[next_pid].state != 0) {
        // Cambiar a idle
        processes[current_pid].stack_pointer = current_rsp;
        processes[0].state = 1;  // idle siempre está listo
        current_pid = 0;
        return processes[0].stack_pointer;
    }
    
    // Cambiar al siguiente proceso
    processes[current_pid].state = 0;  // ready
    processes[next_pid].state = 1;     // running
    current_pid = next_pid;
    last_switch_tick = current_tick;
    
    return processes[current_pid].stack_pointer;
}

void idle_process() {
    while(1) {
        _hlt();  // Halt hasta próxima interrupción
    }
}