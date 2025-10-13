
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
SOURCES_DRIVERS=$(wildcard drivers/*.c)
SOURCES_SYS=$(wildcard sys/*.c)
SOURCES_MM=$(wildcard mm/*.c)
SOURCES_PROC=$(wildcard proc/*.c)
SOURCES_UTILS=$(wildcard utils/*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_DRIVERS=$(SOURCES_DRIVERS:.c=.o) 
OBJECTS_SYS=$(SOURCES_SYS:.c=.o)
OBJECTS_MM=$(SOURCES_MM:.c=.o)
OBJECTS_PROC=$(SOURCES_PROC:.c=.o)
OBJECTS_UTILS=$(SOURCES_UTILS:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
LOADERSRC=loader.asm

LOADEROBJECT=$(LOADERSRC:.asm=.o)
STATICLIBS=

all: $(KERNEL)

$(KERNEL): $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_DRIVERS) $(OBJECTS_SYS) $(OBJECTS_MM) $(OBJECTS_PROC) $(OBJECTS_UTILS) $(STATICLIBS) $(OBJECTS_ASM)
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_DRIVERS) $(OBJECTS_SYS) $(OBJECTS_MM) $(OBJECTS_PROC) $(OBJECTS_UTILS) $(OBJECTS_ASM) $(STATICLIBS)
	$(LD) $(LDFLAGS) -T kernel.ld --oformat=elf64-x86-64 -o kernel.elf $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_DRIVERS) $(OBJECTS_SYS) $(OBJECTS_MM) $(OBJECTS_PROC) $(OBJECTS_UTILS) $(OBJECTS_ASM) $(STATICLIBS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -I./drivers -I./sys -I./mm -I./proc -I./utils -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT):
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

clean:
	rm -rf asm/*.o *.o *.bin drivers/*.o sys/*.o mm/*.o proc/*.o utils/*.o

.PHONY: all clean
