#ifndef COMMANDS_H
#define COMMANDS_H

#define MAX_ARGS 4
#define OK 0
#define ERROR -1
#define EXIT_CODE 1
#define CMD_ERROR -2

typedef int (*cmd_fn)(int argc, char *argv[]);

typedef struct{
    const char *name;
    cmd_fn function;
    const char *help;
    int is_builtin;  // 1 = built-in (no crear proceso), 0 = externo (crear proceso)
}TShellCmd;

extern const TShellCmd shellCmds[];

int helpCmd(int argc, char *argv[]);
int exitCmd(int argc, char *argv[]);
int setUserCmd(int argc, char *argv[]);
int clearCmd(int argc, char *argv[]);
int fontSizeCmd(int argc, char *argv[]);
int testMMCmd(int argc, char *argv[]);
int testProcesesCmd(int argc, char *argv[]);
int testPriorityCmd(int argc, char *argv[]);
int testSyncCmd(int argc, char *argv[]);
int testNoSynchroCmd(int argc, char *argv[]);
int fillCommandAndArgs(char *args[], char *input);  
int CommandParse(char *commandInput);
int exceptionCmd(int argc, char *argv[]);
int regsCmd(int argc, char *argv[]);
int set_some_regs();
int psCmd(int argc, char *argv[]);
int loopCmd(int argc, char *argv[]);
int killCmd(int argc, char *argv[]);
int niceCmd(int argc, char *argv[]);
int blockCmd(int argc, char *argv[]);
int memCmd(int argc, char *argv[]);
int catCmd(int argc, char *argv[]);
int wcCmd(int argc, char *argv[]);
int echoCmd(int argc, char *argv[]);
int filterCmd(int argc, char *argv[]);
int mvarCmd(int argc, char *argv[]);
#endif
