#include <lib.h>
#include <moduleLoader.h>
#include "keyboardDriver.h"
#include "scheduler.h"
#include "mm.h"
#include <idtLoader.h>
#include "process.h"
#include "interrupts.h"
#include "pipe.h"

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
	pipe_system_init();
    keyboard_init();

	scheduler_spawn_process("shell", (void*)sampleCodeModuleAddress, NULL, NULL, 2, 1, 0, 0);

    load_idt();
	_sti();

	while(1){ _hlt(); }

    return 0;
}
