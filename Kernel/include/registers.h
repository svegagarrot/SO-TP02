#ifndef REGISTERS_H

#include <stdint.h>

#define REGISTERS_H

typedef struct {
	uint64_t rax, rbx, rcx, rdx, rbp, rdi, rsi, r8, r9, r10, r11, r12, r13, r14, r15, rip, rsp, rflags;
} CPURegisters;

#endif
