// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "../include/commands.h"
#include <stddef.h>

int g_run_in_background = 0;

const TShellCmd shellCmds[] = {
	{"help", helpCmd, ": Muestra los comandos disponibles\n", 1},
	{"exit", exitCmd, ": Salir del shell\n", 1},
	{"set-user", setUserCmd, ": Setea el nombre de usuario, con un maximo de 10 caracteres\n", 1},
	{"clear", clearCmd, ": Limpia la pantalla\n", 1},
	{"testmm", testMMCmd, ": Ejecuta el stress test de memoria. Uso: testmm <max_mem>\n", 0},
	{"test_proceses", testProcesesCmd, ": Ejecuta el stress test de procesos. Uso: test_proceses <max_proceses>\n", 0},
	{"test_synchro", testSyncCmd, ": Ejecuta test sincronizado con semaforos. Uso: test_synchro <repeticiones>\n", 0},
	{"test_no_synchro", testNoSynchroCmd, ": Ejecuta test sin sincronizacion. Uso: test_no_synchro <repeticiones>\n",
	 0},
	{"test_priority", testPriorityCmd, ": Ejecuta el test de prioridades. Uso: test_priority <max_value>\n", 0},
	{"exceptions", exceptionCmd,
	 ": Testear excepciones. Ingrese: exceptions [zero/invalidOpcode] para testear alguna operacion\n", 1},
	{"mmtype", mmTypeCmd, ": Muestra el tipo de memory manager activo\n", 0},
	{"ps", psCmd, ": Lista todos los procesos con sus propiedades\n", 0},
	{"loop", loopCmd, ": Crea un proceso que imprime su ID con saludo cada X segundos. Uso: loop <segundos>\n", 0},
	{"kill", killCmd, ": Mata un proceso por PID. Uso: kill <pid>\n", 1},
	{"nice", niceCmd, ": Cambia la prioridad de un proceso. Uso: nice <pid> <prioridad>\n", 1},
	{"block", blockCmd, ": Cambia el estado de un proceso entre bloqueado y listo. Uso: block <pid>\n", 1},
	{"mem", memCmd, ": Imprime el estado de la memoria\n", 0},
	{"cat", catCmd, ": Imprime el stdin tal como lo recibe\n", 0},
	{"wc", wcCmd, ": Cuenta la cantidad de lineas del input\n", 0},
	{"filter", filterCmd, ": Filtra las vocales del input\n", 0},
	{"mvar", mvarCmd, ": Simula MVar con lectores/escritores. Uso: mvar <escritores> <lectores>\n", 0},
	{NULL, NULL, NULL, 0},
};
