#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>

#define MAX_ARGS 4
#define OK 0
#define ERROR -1
#define EXIT_CODE 1
#define CMD_ERROR -2
#define SHELL_PID 1

typedef int (*cmd_fn)(int argc, char *argv[]);

typedef struct{
    const char *name;
    cmd_fn function;
    const char *help;
    int is_builtin;  // 1 = built-in (no crear proceso), 0 = externo (crear proceso)
}TShellCmd;

extern const TShellCmd shellCmds[];

// ===== commands_builtin.c =====
int helpCmd(int argc, char *argv[]);
int exitCmd(int argc, char *argv[]);
int setUserCmd(int argc, char *argv[]);
int clearCmd(int argc, char *argv[]);
int exceptionCmd(int argc, char *argv[]);
int killCmd(int argc, char *argv[]);
int niceCmd(int argc, char *argv[]);
int blockCmd(int argc, char *argv[]);

// ===== commands_tests.c =====
int testMMCmd(int argc, char *argv[]);
int testProcesesCmd(int argc, char *argv[]);
int testPriorityCmd(int argc, char *argv[]);
int testSyncCmd(int argc, char *argv[]);
int testNoSynchroCmd(int argc, char *argv[]);

// ===== commands_system.c =====
int psCmd(int argc, char *argv[]);
int loopCmd(int argc, char *argv[]);
int memCmd(int argc, char *argv[]);
int mmTypeCmd(int argc, char *argv[]);

// ===== commands_pipe.c =====
int catCmd(int argc, char *argv[]);
int wcCmd(int argc, char *argv[]);
int filterCmd(int argc, char *argv[]);
int mvarCmd(int argc, char *argv[]);

// ===== command_parser.c =====
int fillCommandAndArgs(char *args[], char *input);  
int CommandParse(char *commandInput);
int64_t execute_external_command(const char *name, void *function, char *argv[], 
                                  int is_foreground, uint64_t stdin_pipe_id, uint64_t stdout_pipe_id);
void *get_process_entry_function(int cmd_idx);

// ===== process_wrappers.c =====
const char *state_to_string(int state);
void test_mm_process_wrapper(void *arg);
void test_processes_wrapper(void *arg);
void test_prio_wrapper(void *arg);
void test_sync_wrapper(void *arg);
void test_no_synchro_wrapper(void *arg);
void loop_process_entry(void *arg);
void ps_process_entry(void *arg);
void cat_process_entry(void *arg);
void wc_process_entry(void *arg);
void filter_process_entry(void *arg);
void mvar_writer_entry(void *arg);
void mvar_reader_entry(void *arg);
void mem_process_entry(void *arg);
void mmtype_process_entry(void *arg);

#endif
