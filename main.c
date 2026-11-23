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

int read_input(char **line, size_t *capacity){
    printf("shellite$ ");

    if(getline(line, capacity, stdin) == -1){
        return 0;
    }
    
    return 1;
}

int tokenize(char *line, char *commands[][MAX_ARGS]){
    int cmd = 0;
    int arg = 0;
    char *token;

    token = strtok(line, " \n\t");
    while(token != NULL){
        if(strcmp(token, "|") == 0){
            if(arg == 0){
                printf("empty pipeline\n");
                return 0;
            }else if(cmd >= MAX_COMMANDS-1){
                printf("too many commands\n");
                return 0;
            }
            commands[cmd][arg] = NULL;
            cmd++;
            arg = 0;
        }else{
            if(arg >= MAX_ARGS-1){
                printf("too many arguements\n");
                return 0;
            }
            commands[cmd][arg++] = token;
        }
        token = strtok(NULL, " \n\t");
    }
    commands[cmd][arg] = NULL;

    if(arg==0){
        printf("empty pipeline\n");
        return 0;
    }

    return cmd+1;
}

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

void exec_commands(char **args){
    pid_t pid = fork();
    if(pid==-1){
        perror("fork");
    }else if(pid==0){
        execvp(args[0], args);
        perror(args[0]);
        _exit(127);
    }else{
        waitpid(pid, NULL, 0);
        return;
    }
}

void exec_pipelines(int cmd_count, char *commands[][MAX_ARGS]){
    int prev_read = -1;
    pid_t pids[MAX_COMMANDS];
    for(int i=0;i<cmd_count;i++){
        int fd[2] = {-1, -1};
        if(i < cmd_count-1 && pipe(fd)==-1){
            perror("pipe");
            break;
        }

        pid_t pid = fork();
        if(pid==-1){
            perror("fork");
            break;
        }else if(pid==0){
            if(prev_read!=-1){
                dup2(prev_read, STDIN_FILENO);
            }
            
            if(fd[1]!=-1){
                dup2(fd[1],STDOUT_FILENO);
            }

            if(prev_read!=-1){
                close(prev_read);
            }
            
            close(fd[1]);
            close(fd[0]);
            execvp(commands[i][0], commands[i]);
            perror(commands[i][0]);
            _exit(127);
        }else{
            pids[i] = pid;
            if(prev_read!=-1){
                close(prev_read);
            }
            close(fd[1]);
            prev_read = fd[0];
        }
    }

    for(int i=0;i<cmd_count;i++){
        waitpid(pids[i], NULL, 0);
    }
}

int main(void) {
    char *line = NULL;
    size_t capacity = 0;

    char* commands[MAX_COMMANDS][MAX_ARGS];
    int command_count;

    while (1) {
        if(read_input(&line, &capacity) == 0){
            printf("error or EOF encountered. exiting...");
            break;
        }

        command_count = tokenize(line, commands);
        if(command_count == 0){
            continue;
        } 

        if(command_count==1 && run_builtin(commands[0])){
            continue;
        }

        if(command_count==1){
            exec_commands(commands[0]);
        }else{
            exec_pipelines(command_count, commands);
        }
    }

    free(line);
    return 0;
}