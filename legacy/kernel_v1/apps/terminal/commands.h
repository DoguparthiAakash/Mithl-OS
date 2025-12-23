#ifndef TERMINAL_COMMANDS_H
#define TERMINAL_COMMANDS_H

#include "terminal.h"

/* Command handler function type */
typedef void (*cmd_handler_t)(terminal_t *term, const char *args);

/* Command structure */
typedef struct {
    const char *name;
    cmd_handler_t handler;
    const char *description;
} command_t;

/* Command handlers */
void cmd_shutdown(terminal_t *term, const char *args);
void cmd_restart(terminal_t *term, const char *args);
void cmd_halt(terminal_t *term, const char *args);
void cmd_echo(terminal_t *term, const char *args);
void cmd_uname(terminal_t *term, const char *args);
void cmd_free(terminal_t *term, const char *args);
void cmd_uptime(terminal_t *term, const char *args);
void cmd_mkdir(terminal_t *term, const char *args);
void cmd_rm(terminal_t *term, const char *args);
void cmd_touch(terminal_t *term, const char *args);
void cmd_cp(terminal_t *term, const char *args);
void cmd_mv(terminal_t *term, const char *args);

/* Command execution */
void execute_command(terminal_t *term, const char *cmdline);

/* Get command list */
const command_t* get_commands(void);

#endif /* TERMINAL_COMMANDS_H */
