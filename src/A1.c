
/* 
* Alexander Harris - 260688155
* ESCSE 427 - Assignment 1
* Simple Shell
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_ARGS 40

//TODO: Set up output redirection, jobs and background function, sig handling.

int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;

    //Resets args array
    for (int j = 0; j < MAX_ARGS; j++){
        args[j] = '\0';
    }

    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);
    
    if(length <= 0){
        exit(-1);
    }

    if((loc = index(line, '&')) != NULL){
        *background = 1;
        *loc = ' ';
    }
    else{
        *background = 0;
    }

    while((token = strsep(&line, " \t\n")) != NULL){
        for(int j = 0; j < strlen(token); j++){
            if(token[j] <= 32){
                token[j] = '\0';
            }
        }
        if(strlen(token) > 0){
            args[i++] = token;
        }
    }
    return i;
}

int main(void)
{
    time_t now;
    srand((unsigned int) (time(&now)));

    char *args[MAX_ARGS];
    int bg;

    while(1){
        bg = 0;
        int cnt = getcmd("\n>> ", args, &bg);

        if(cnt <= 0){
            continue;
        }
        
        //Increases execution time for syscalls
        /*int w, rem;
        w =  rand() % 10;
        rem = sleep(w);
        while(rem != 0){
            rem = sleep(rem);
        }*/

        exec(args, bg);
    }
}

void exec(char* args[], int bg)
{
    pid_t pid;

    if(strcmp(args[0], "exit") == 0){
        exit(0);
    }
    else if(strcmp(args[0], "cd") == 0){
        int dir = chdir(args[1]);
        if(dir != 0){
            printf("Invalid directory.\n");
        }
    }
    else{
        pid = fork();

        if(pid == 0){
            execvp(args[0], args);
        }
        else if(pid > 0){
            waitpid(0, NULL, 0);
        }
        else{
            perror("Fork failed.\n");
            exit(-1);
        }
    }
}