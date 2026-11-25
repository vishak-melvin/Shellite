#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define MAX_COMMANDS 16
#define MAX_ARGS 128
#define MAX_JOBS 10

typedef struct {
    int id;
    pid_t pid;
    int active;
}job;
job jobs[MAX_JOBS];
int jcount = 0;

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

void show_jobs(){
    for(int i=0;i<MAX_JOBS;i++){
        if(jobs[i].active == 0){
            continue;
        }

        int status = waitpid(jobs[i].pid,NULL,WNOHANG);
        if(status==0){
            printf("[%d] PID: %d\n",jobs[i].id,jobs[i].pid);
        }else{
            jobs[i].active = 0;
        }
    }
}

void kill_job(int id){
    for(int i=0;i<MAX_JOBS;i++){
        if(jobs[i].id == id && jobs[i].active == 1){
            if(kill(jobs[i].pid, SIGTERM) == -1){
                perror("kill");
            }
            return;
        }
    }
    printf("job not found");
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

    if(strcmp(args[0], "jobs") == 0){
        show_jobs();
        return 1;
    }

    if(strcmp(args[0], "kill") == 0){
        int id = atoi(args[1]);
        kill_job(id);
        return 1;
    }
    return 0;
}

void exec_commands(char **args, int background){
    pid_t pid = fork();
    if(pid==-1){
        perror("fork");
    }else if(pid==0){
        signal(SIGINT, SIG_DFL);

        execvp(args[0], args);
        perror(args[0]);
        _exit(127);
    }else{
        if(!background){
            waitpid(pid, NULL, 0);
        }else{
            int available = -1;
            for(int i=0;i<MAX_JOBS;i++){
                if(jobs[i].active == 0){
                    available = i;
                    break;
                }
            }
            if(available!=-1){
                jobs[available].id = jcount++;
                jobs[available].pid = pid;
                jobs[available].active = 1;
            }
        }
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
            signal(SIGINT,SIG_DFL);

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
    signal(SIGINT, SIG_IGN);
    char *line = NULL;
    size_t capacity = 0;

    char* commands[MAX_COMMANDS][MAX_ARGS];
    int command_count;

    while(1){
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

        int length = 0;
        while(commands[0][length] != NULL){
            length++;
        }

        if(command_count==1){
            if(strcmp(commands[0][length-1],"&") == 0){
                commands[0][length-1] = NULL;
                exec_commands(commands[0],1);
            }else{
                exec_commands(commands[0],0);
            }
        }else{
            exec_pipelines(command_count, commands);
        }
    }

    free(line);
    return 0;
}