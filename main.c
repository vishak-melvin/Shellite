// use POSIX.1-2008 interface
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_COMMANDS 16
#define MAX_ARGS 128

// detect and run built in commands like exit or cd which shellite must handle itself
// this means that they should not be run by a child process but rather by the parent itself
int run_builtin(char **args){
    // exit command
    if(strcmp(args[0], "exit") == 0){
        exit(0);
    }

    // cd(change directory) command
    if(strcmp(args[0], "cd") == 0){
        const char *path = args[1];

        if(path == NULL){
            path = getenv("HOME");
        }

        if(path == NULL){
            printf("cd: HOME is not set\n");
        }else if(chdir(path) == -1){
            perror("cd");
        }

        return 1;
    }
    return 0;
}

int main(){
    char *line = NULL; // the line the user typed
    size_t capacity = 0; // allocated size of input buffer

    while(1){
        printf("shellite$ ");

        if(getline(&line, &capacity, stdin) == -1){
            if(feof(stdin)){ // handles Ctrl+D that returns EOF to the shell
                break;
            }
            perror("getLine");
            continue;
        }

        // tokenizer
        char *commands[MAX_COMMANDS][MAX_ARGS]; // hold full commands eg "printf hello"
        int arg_counts[MAX_COMMANDS] = {0}; // hold arguemnets of commands eg "printf", "hello"
        int command_count = 1; // number of commands separated by pipeline |

        char *token = strtok(line, " \t\n");
        int invalid = 0;

        // add NULL at the end of last valid token
        while(token != NULL){
            if(strcmp(token, "|") == 0){
                // handle empty "|" or is max command is reached
                if(arg_counts[command_count - 1] == 0 || command_count == MAX_COMMANDS){
                    invalid = 1;
                    break;
                }
                command_count++;
            }else{
                int current = command_count - 1;
    
                if(arg_counts[current] >= MAX_ARGS - 1){
                    printf("too many arguements\n");
                    invalid = 1;
                    break;
                }
                commands[current][arg_counts[current]++] = token;
            }
            token = strtok(NULL, " \t\n");
        }

        // invalid command
        if(invalid){
            printf("invalid command\n");
            continue;
        }

        // handle empty input
        if(arg_counts[0]==0){
            continue;
        }

        // reject empty pipeline
        if(arg_counts[command_count - 1] == 0){
            printf("invalid pipeline\n");
            continue;
        }
        
        // init end of command[i] to be NULL
        for(int i =0; i < command_count; i++){
            commands[i][arg_counts[i]] = NULL;
        }

        // use built in command if available
        if(command_count == 1 && run_builtin(commands[0])){
            continue;
        }

        // spawn child process that will run external programs like ls, pwd, grep etc.
        pid_t pids[MAX_COMMANDS];
        int started = 0; // children created
        int previous_read = -1; // previous pipe read
        int failed = 0; // failed children

        for(int i = 0;i < command_count;i++){
            int pipe_fds[2] = {-1, -1}; // pipe_fds[0] == pipe to read from, pipe_fds[1] == pipe to write to

            // only create a pipe if there are multiple commands
            if(i < command_count - 1 && pipe(pipe_fds) == -1){
                perror("pipe");
                failed = 1;
                break;
            }

            pid_t pid = fork();

            // exit and clean pipes
            if(pid == -1){
                perror("fork");
                if(pipe_fds[0] != -1)
                    close(pipe_fds[0]);
                if(pipe_fds[1] != -1)
                    close(pipe_fds[1]);
                
                failed = 1;
                break;
            }

            if(pid == 0){
                if(previous_read != -1){
                    // set old file descriptor to 1
                    if(dup2(previous_read, STDIN_FILENO) == -1){
                        perror("dup2");
                        _exit(1);
                    }
                }

                if(pipe_fds[1] != -1){
                    if(dup2(pipe_fds[1], STDOUT_FILENO) == -1){
                        perror("dup2");
                        _exit(1);
                    }
                }

                if (previous_read != -1)
                    close(previous_read);
                if (pipe_fds[0] != -1)
                    close(pipe_fds[0]);
                if (pipe_fds[1] != -1)
                    close(pipe_fds[1]);

                execvp(commands[i][0], commands[i]); // run command if available in PATH. only returns if failed
                perror(commands[i][0]);
                _exit(127); // standard code for command failure
            }

            // save pid of child
            pids[started++] = pid;

            if(previous_read != -1){
                close(previous_read);
            }

            if(pipe_fds[1] != -1){
                close(pipe_fds[1]);
            }

            previous_read = pipe_fds[0];
        }

        if(previous_read != -1){
            close(previous_read);
        }

        for(int i = 0; i < started; i++){
            while(waitpid(pids[i], NULL, 0) == -1){
                if(errno != EINTR){
                    perror("waitpid");
                    break;
                }
            }
        }
        (void)failed;
    }

    // free line and move to next line after output
    free(line);
    putchar('\n');
    return 0;
}