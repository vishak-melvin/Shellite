// use POSIX.1-2008 interface
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


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
            fprintf(stderr, "cd: HOME is not set\n");
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

    for(;;){ // infinite loop
        printf("shellite$ ");
        fflush(stdout); // flush output so above line shows immediately when waiting for input

        if(getline(&line, &capacity, stdin) == -1){
            if(feof(stdin)){ // handles Ctrl+D that returns EOF to the terminal
                break;
            }
            perror("getLine");
            continue;
        }

        // tokenizer
        char *args[128];
        size_t count = 0;
        char *saveptr;
        char *token = strtok_r(line, " \t\n", &saveptr);

        // add NULL at the end of last valid token
        while(token != NULL && count < 127){
            args[count++] = token;
            token = strtok_r(NULL, " \t\n", &saveptr);
        }
        args[count] = NULL;

        // handle empty input
        if(count == 0){
            continue;
        }

        // use built in command if available
        if(run_builtin(args)){
            continue;
        }

        // spawn child process that will run programs like ls, pwd, grep etc.
        pid_t pid = fork();
        if(pid == -1){
            perror("fork");
            continue;
        }

        if(pid == 0){
            execvp(args[0], args); // handles external shell commands like ls,pwd etc by searching PATH for it
            perror(args[0]); // return error if execvp couldnt find the command
            _exit(127); // 127 is standard error status for "command nor found"
        }
        
        // waits for child process to complete command
        int status;
        while(waitpid(pid, &status, 0) == -1){
            if(errno != EINTR){
                perror("waitpid");
                break;
            }
        }
    }

    // free line and move to next line after output
    free(line);
    putchar('\n');
    return 0;
}