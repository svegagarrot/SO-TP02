
include Makefile.inc

# Memory manager selection: MM=simple (default) or MM=buddy
MM ?= simple

ifeq ($(MM),buddy)
GCCFLAGS += -DUSE_BUDDY_MM
else
GCCFLAGS += -DUSE_SIMPLE_MM
endif

KERNEL=kernel.bin
SOURCES=$(wildcard *.c)
SOURCES_EXCEPTIONS=$(wildcard exceptions/*.c)
SOURCES_INTERRUPTS=$(wildcard interrupts/*.c)
SOURCES_LIB=$(wildcard lib/*.c)
SOURCES_LOADER=$(wildcard loader/*.c)
SOURCES_CONSOLE=$(wildcard console/*.c)
SOURCES_TIME=$(wildcard time/*.c)
SOURCES_DRIVERS=$(wildcard drivers/*.c)
SOURCES_SYSCALLS=$(wildcard syscalls/*.c)
SOURCES_IPC=$(wildcard ipc/*.c)
SOURCES_PROC=$(wildcard proc/*.c)
SOURCES_UTILS=$(wildcard utils/*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_EXCEPTIONS=$(SOURCES_EXCEPTIONS:.c=.o)
OBJECTS_INTERRUPTS=$(SOURCES_INTERRUPTS:.c=.o)
OBJECTS_LIB=$(SOURCES_LIB:.c=.o)
OBJECTS_LOADER=$(SOURCES_LOADER:.c=.o)
OBJECTS_CONSOLE=$(SOURCES_CONSOLE:.c=.o)
OBJECTS_TIME=$(SOURCES_TIME:.c=.o)
OBJECTS_DRIVERS=$(SOURCES_DRIVERS:.c=.o) 
OBJECTS_SYSCALLS=$(SOURCES_SYSCALLS:.c=.o)
OBJECTS_IPC=$(SOURCES_IPC:.c=.o)
# Build only the selected memory manager to avoid linking both implementations
ifeq ($(MM),buddy)
OBJECTS_MM=mm/mm_buddy.o
else
OBJECTS_MM=mm/mm_simple.o
endif
OBJECTS_PROC=$(SOURCES_PROC:.c=.o)
OBJECTS_UTILS=$(SOURCES_UTILS:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
LOADERSRC=boot/loader.asm

LOADEROBJECT=boot/loader.o
STATICLIBS=

all: $(KERNEL)

$(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_EXCEPTIONS) $(OBJECTS_INTERRUPTS) $(OBJECTS_LIB) $(OBJECTS_LOADER) $(OBJECTS_CONSOLE) $(OBJECTS_TIME) $(OBJECTS_DRIVERS) $(OBJECTS_SYSCALLS) $(OBJECTS_IPC) $(OBJECTS_MM) $(OBJECTS_PROC) $(OBJECTS_UTILS) $(STATICLIBS) $(OBJECTS_ASM)
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_EXCEPTIONS) $(OBJECTS_INTERRUPTS) $(OBJECTS_LIB) $(OBJECTS_LOADER) $(OBJECTS_CONSOLE) $(OBJECTS_TIME) $(OBJECTS_DRIVERS) $(OBJECTS_SYSCALLS) $(OBJECTS_IPC) $(OBJECTS_MM) $(OBJECTS_PROC) $(OBJECTS_UTILS) $(OBJECTS_ASM) $(STATICLIBS)
	$(LD) $(LDFLAGS) -T kernel.ld --oformat=elf64-x86-64 -o kernel.elf $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_EXCEPTIONS) $(OBJECTS_INTERRUPTS) $(OBJECTS_LIB) $(OBJECTS_LOADER) $(OBJECTS_CONSOLE) $(OBJECTS_TIME) $(OBJECTS_DRIVERS) $(OBJECTS_SYSCALLS) $(OBJECTS_IPC) $(OBJECTS_MM) $(OBJECTS_PROC) $(OBJECTS_UTILS) $(OBJECTS_ASM) $(STATICLIBS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT):
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

clean:
	rm -rf asm/*.o *.o *.bin boot/*.o exceptions/*.o interrupts/*.o lib/*.o loader/*.o console/*.o time/*.o drivers/*.o syscalls/*.o ipc/*.o mm/*.o proc/*.o utils/*.o

.PHONY: all clean
