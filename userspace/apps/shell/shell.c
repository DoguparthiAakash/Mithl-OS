#include "stdlib.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

// Minimal printf (only supports %s and %c for now, and int)


// Minimal printf (only supports %s and %c for now, and int)
void printf_shim(const char *format, ...) {
    // We don't have stdarg.h?
    // Userspace might not have it.
    // Let's just use simple print calls for now.
    // If we need formatting, we'll do manual.
    print(format);
}

// Better: Split usage directly in main
#define MAX_CMD_LEN 256
#define MAX_ARGS 32

int main(int argc, char **argv) {
    char cmdline[MAX_CMD_LEN];
    (void)argc; (void)argv;

    print("\nWelcome to Mithl-OS Shell (Advanced)\n");
    print("Type 'help' for commands.\n");

    while (1) {
        print("$ ");
        
        // Read line
        memset(cmdline, 0, MAX_CMD_LEN);
        int i = 0;
        while (i < MAX_CMD_LEN - 1) {
            char c;
            int n = read(0, &c, 1);
            if (n > 0) {
                if (c == '\n') {
                    print("\n");
                    break;
                } else if (c == '\b') {
                    if (i > 0) {
                        i--;
                        print("\b \b");
                    }
                } else {
                    cmdline[i++] = c;
                    char tmp[2] = {c, 0};
                    print(tmp);
                }
            }
        }
        cmdline[i] = 0;

        if (strlen(cmdline) == 0) continue;

        // Tokenize for Pipe first
        char *pipe_parts[2] = {0};
        int pipe_count = 0;
        
        for(int k=0; k<strlen(cmdline); k++) {
            if (cmdline[k] == '|') {
                cmdline[k] = 0;
                pipe_parts[0] = cmdline;
                pipe_parts[1] = cmdline + k + 1;
                pipe_count = 1;
                break;
            }
        }
        
        if (pipe_count == 0) pipe_parts[0] = cmdline;

        if (pipe_count > 0) {
            int pfd[2];
            if (pipe(pfd) == -1) {
                print("Pipe failed\n");
                continue;
            }

            int pid1 = fork();
            if (pid1 == 0) {
                close(1);
                dup2(pfd[1], 1);
                close(pfd[0]);
                close(pfd[1]);
                
                 char *args[MAX_ARGS];
                 int argc_cmd = 0;
                 char *token = strtok(pipe_parts[0], " ");
                 while(token && argc_cmd < MAX_ARGS-1) {
                     args[argc_cmd++] = token;
                     token = strtok(0, " ");
                 }
                 args[argc_cmd] = 0;
                 
                 char path[64];
                 if (args[0][0] == '/') strcpy(path, args[0]);
                 else { strcpy(path, "/bin/"); strcat(path, args[0]); }
                 
                 execve(path, args, 0);
                 if (args[0][0] != '/') {
                      strcpy(path, "/"); strcat(path, args[0]);
                      execve(path, args, 0);
                 }
                 print("Command not found\n");
                 exit(1);
            }

            int pid2 = fork();
            if (pid2 == 0) {
                close(0);
                dup2(pfd[0], 0);
                close(pfd[1]);
                close(pfd[0]);
                
                 char *args[MAX_ARGS];
                 int argc_cmd = 0;
                 char *token = strtok(pipe_parts[1], " ");
                 while(token && argc_cmd < MAX_ARGS-1) {
                     args[argc_cmd++] = token;
                     token = strtok(0, " ");
                 }
                 args[argc_cmd] = 0;
                 
                 char path[64];
                 if (args[0][0] == '/') strcpy(path, args[0]);
                 else { strcpy(path, "/bin/"); strcat(path, args[0]); }
                 
                 execve(path, args, 0);
                 if (args[0][0] != '/') {
                      strcpy(path, "/"); strcat(path, args[0]);
                      execve(path, args, 0);
                 }
                 print("Command not found\n");
                 exit(1);
            }

            close(pfd[0]);
            close(pfd[1]);
            waitpid(pid1, 0, 0);
            waitpid(pid2, 0, 0);
            continue;
        }

        // REDIRECTION LOGIC
        char *redir_file = 0;
        int append = 0;
        for(int k=0; k<strlen(cmdline); k++) {
             if (cmdline[k] == '>') {
                 if (cmdline[k+1] == '>') {
                     append = 1;
                     cmdline[k] = 0;
                     redir_file = cmdline + k + 2;
                 } else {
                     cmdline[k] = 0;
                     redir_file = cmdline + k + 1;
                 }
                 break;  
             }
        }

        char *args[MAX_ARGS];
        int argc_cmd = 0;
        char *token = strtok(cmdline, " ");
        while (token != 0 && argc_cmd < MAX_ARGS - 1) {
            args[argc_cmd++] = token;
            token = strtok(0, " ");
        }
        args[argc_cmd] = 0;

        if (argc_cmd == 0) continue;

        if (strcmp(args[0], "exit") == 0) {
            print("Exiting Shell.\n");
            exit(0);
        }
        if (strcmp(args[0], "cd") == 0) {
            if (argc_cmd < 2) {
                // cd home?
                if (chdir("/") != 0) print("cd: failed\n");
            } else {
                if (chdir(args[1]) != 0) {
                     print("cd: no such file or directory: ");
                     print(args[1]);
                     print("\n");
                }
            }
            continue;
        }
        if (strcmp(args[0], "help") == 0) {
             print("Commands:\n");
             print("  ls, cat, ps, cp, mv, mkdir\n");
             print("  cmd > file, cmd1 | cmd2\n");
             print("  semantic <query> : AI Agent Action\n");
             continue;
        }

        if (strcmp(args[0], "semantic") == 0) {
            if (argc_cmd < 2) {
                print("Usage: semantic <query> | register <name> <tags> <path>\n");
                continue;
            }
            
            // Subcommand: register
            if (strcmp(args[1], "register") == 0) {
                if (argc_cmd < 5) {
                    print("Usage: semantic register <name> <tags> <path>\n");
                    continue;
                }
                
                agent_node_t op;
                strcpy(op.name, args[2]);
                strcpy(op.intents, args[3]);
                strcpy(op.binary, args[4]);
                
                if (agent_op(AGENT_OP_REGISTER, &op, 0) == 0) {
                    print("Agent Registered Successfully.\n");
                } else {
                    print("Registration Failed.\n");
                }
                continue;
            }
            
            // Default: Query
            // Reconstruct query from args
            char query[128]; 
            query[0] = 0;
            for(int j=1; j<argc_cmd; j++) {
                strcat(query, args[j]);
                if (j < argc_cmd-1) strcat(query, " ");
            }
            
            char binary[128];
            if (agent_op(AGENT_OP_QUERY, query, binary) == 0) {
                print("AI Agent found: "); print(binary); print("\n");
                
                int pid = fork();
                if (pid == 0) {
                    char *new_argv[2];
                    new_argv[0] = binary; 
                    new_argv[1] = 0;
                    
                    // Path heuristics
                    char path[64];
                    if (binary[0] == '/') strcpy(path, binary);
                    else { strcpy(path, "/bin/"); strcat(path, binary); }
                    
                    execve(path, new_argv, 0);
                    
                    if (binary[0] != '/') {
                        strcpy(path, "/"); strcat(path, binary);
                        execve(path, new_argv, 0);
                    }
                    
                    print("Execution failed.\n");
                    exit(1);
                } else {
                    waitpid(pid, 0, 0);
                }
            } else {
                print("No agent found for that intent.\n");
            }
            continue;
        }

        int pid = fork();
        if (pid == 0) {
            if (redir_file) {
                 while(*redir_file == ' ') redir_file++;
                 close(1);
                 // Need generic creating open logic or just create file via syscall
                 // For now, hack: try creating file if create syscall existed or just reuse open
                 // open(path, 0)
                 // Just assume file exists for redirection test or use 'creat' if I added it
                 // I added 'creat' to stdlib.h (int creat(const char*, mode))
                 creat(redir_file, 0); // Ensure exists
                 int fd = open(redir_file, 0); // Open
                 if (fd != 1) {
                     dup2(fd, 1);
                     if (fd != 1) close(fd);
                 }
            }
            
            char path[64];
            if (args[0][0] == '/') strcpy(path, args[0]);
            else { 
                // Manual sprintf equivalent
                strcpy(path, "/bin/");
                strcat(path, args[0]);
            }
            
            execve(path, args, 0); 
            
            if (args[0][0] != '/') {
                strcpy(path, "/"); strcat(path, args[0]);
                execve(path, args, 0);
                
                strcpy(path, "/bin/"); strcat(path, args[0]); strcat(path, ".elf");
                execve(path, args, 0);
                
                strcpy(path, "/"); strcat(path, args[0]); strcat(path, ".elf");
                execve(path, args, 0);
            }
            
            print("Command not found\n");
            exit(1);
        } else {
             waitpid(pid, 0, 0);
        }
    }
    return 0;
}
