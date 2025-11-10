// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <exceptions.h>

static void video_printRegister(char *regName, uint64_t regValue);
static void video_printRegisters(uint64_t exceptionRegisters[18]);

static char *regs[] = {"RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "RBP", "RSP", "R8",
					   "R9",  "R10", "R11", "R12", "R13", "R14", "R15", "RIP", "RFLAGS"};

void exceptionDispatcher(int exception, uint64_t exceptionRegisters[18]) {
	if (exception == ZERO_EXCEPTION_ID)
		zero_division();
	else if (exception == INVALID_OPCODE_ID) {
		invalidOperation();
	}
	video_newLine();
	video_printRegisters(exceptionRegisters);
}

void zero_division() {
	video_printError("Error: No esta permitida la division por cero");
	video_newLine();
}

void invalidOperation() {
	video_printError("Error: Operacion invalida");
	video_newLine();
}

void video_printRegister(char *regName, uint64_t regValue) {
	video_putString(regName, 0xFFFFFF, 0x000000);
	video_putString(": 0x", 0xFFFFFF, 0x000000);

	char hexDigits[] = "0123456789ABCDEF";
	char buffer[17];
	buffer[16] = '\0';
	for (int i = 15; i >= 0; i--) {
		buffer[i] = hexDigits[regValue & 0xF];
		regValue >>= 4;
	}
	video_putString(buffer, 0xFFFFFF, 0x000000);

	video_newLine();
}

void video_printRegisters(uint64_t exceptionRegisters[18]) {
	for (int i = 0; i < 18; i++) {
		video_printRegister(regs[i], exceptionRegisters[i]);
	}
}
