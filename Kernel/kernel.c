#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <moduleLoader.h>
#include <naiveConsole.h>
#include "videoDriver.h"
#include "keyboardDriver.h"
#include "scheduler.h"
#include "mm.h"
#include <idtLoader.h>
#include "process.h"
#include "interrupts.h"

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * const sampleCodeModuleAddress = (void*)0x400000;
static void * const sampleDataModuleAddress = (void*)0x500000;

typedef int (*EntryPoint)();

void clearBSS(void * bssAddress, uint64_t bssSize)
{
    memset(bssAddress, 0, bssSize);
}

void * getStackBase()
{
    return (void*)(
        (uint64_t)&endOfKernel
        + PageSize * 8
        - sizeof(uint64_t)
    );
}

void idle(){
	while(1){
		_hlt();
	}
}

//DESPUES BORRAR
static void cpu_pause(volatile uint64_t iters) {
    while (iters--) { __asm__ __volatile__("pause"); }
}

static void procA(void *arg) {
    (void)arg;
    for (;;) {
        video_putChar('A', FOREGROUND_COLOR, BACKGROUND_COLOR);
        cpu_pause(5000000);
    }
}

static void procB(void *arg) {
    (void)arg;
    for (;;) {
        video_putChar('B', FOREGROUND_COLOR, BACKGROUND_COLOR);
        cpu_pause(5000000);
    }
}

void * initializeKernelBinary()
{
    void * moduleAddresses[] = {
        sampleCodeModuleAddress,
        sampleDataModuleAddress
    };

    loadModules((&endOfKernelBinary), moduleAddresses);

    clearBSS(&bss, (uint64_t)&endOfKernel - (uint64_t)&bss);

    return getStackBase();
}

int main()
{   
	_cli();
	mm_init_default();
	init_scheduler();

	scheduler_spawn_process("A", procA, NULL, NULL);
	scheduler_spawn_process("B", procB, NULL, NULL);
	//scheduler_spawn_process("shell", (void*)sampleCodeModuleAddress, NULL, NULL);	

	load_idt();
	_sti();

	for(;;){ _hlt(); }

    return 0;
}
