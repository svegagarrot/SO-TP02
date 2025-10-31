#ifndef COMMANDS_H
#define COMMANDS_H

#define MAX_ARGS 3
#define OK 0
#define ERROR -1
#define EXIT_CODE 1
#define CMD_ERROR -2

typedef int (*cmd_fn)(int argc, char *argv[]);

typedef struct{
    const char *name;
    cmd_fn function;
    const char *help;
}TShellCmd;

extern const TShellCmd shellCmds[];

int helpCmd(int argc, char *argv[]);
int exitCmd(int argc, char *argv[]);
int setUserCmd(int argc, char *argv[]);
int clearCmd(int argc, char *argv[]);
int timeCmd(int argc, char *argv[]);
int fontSizeCmd(int argc, char *argv[]);
int testMMCmd(int argc, char *argv[]);
int testProcesesCmd(int argc, char *argv[]);
int testPriorityCmd(int argc, char *argv[]);
int testSyncCmd(int argc, char *argv[]);
int testNoSynchroCmd(int argc, char *argv[]);
int fillCommandAndArgs(char *args[], char *input);  
int CommandParse(char *commandInput);
int exceptionCmd(int argc, char *argv[]);
int gameCmd(int argc, char *argv[]);
int regsCmd(int argc, char *argv[]);
int set_some_regs();
#endif
