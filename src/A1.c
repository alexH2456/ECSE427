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
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_ARGS 40

//TODO: Set up output redirection, jobs and background function, sig handling.

int commandNumber = 0;
struct node *head_job = NULL;
struct node *current_job = NULL;
int process_pid;

struct node {   //Linked list object
    int number; 
    int pid;
    struct node *next;
};

void addToJobList(char *args[]) {

    struct node *job = malloc(sizeof(struct node));

    //If the job list is empty, create a new head
    if (head_job == NULL) {
        job->number = 1;
        job->pid = process_pid;

        //the new head is also the current node
        job->next = NULL;
        head_job = job;
        current_job = head_job;
    }

    //Otherwise create a new job node and link the current node to it
    else {

        job->number = current_job->number + 1;
        job->pid = process_pid;

        current_job->next = job;
        current_job = job;
        job->next = NULL;
    }
}

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
    free(line);

    return i;
}

static void sigHandler(int sig)
{
    printf("Caught signal: %d\n", sig);
}

void exec(char* args[], int bg)
{
    pid_t pid;
    
    //Deals with '~' character for Home directory
    for(int i = 0; args[i]; i++){
        if(strchr(args[i], 126)){
            char* home = getenv("HOME");
            char* new;
            char* copy = args[i];

            memmove(&copy[0], &copy[1], strlen(copy));

            if((new = malloc(strlen(copy) + strlen(home) + 1)) != NULL){
                new[0] = '\0';
                strcat(new, home);
                strcat(new, copy);
                args[i] = new;
            }
            else{
                fprintf((stderr), "Mem alloc failed\n");
            }
        }
    }

    if(strcmp(args[0], "exit") == 0){
        exit(0);
    }
    else if(strcmp(args[0], "cd") == 0){
        int dir = chdir(args[1]);
        if(dir != 0){
            printf("Invalid directory.\n");
        }
    }

    //Use execvp for all other commands
    else{
        pid = fork();

        if(pid == 0){
            //Output redirection
            for(int j = 0; args[j]; j++){

                char* redirect = args[j+1];

                if(strcmp(args[j], ">") == 0){
                    close(1);
                    open(redirect, O_WRONLY | O_CREAT | O_TRUNC, 0755);
                    args[j] = args[j+1] = NULL;
                }
                else if(strcmp(args[j], ">>") == 0){
                    close(1);
                    open(redirect, O_WRONLY | O_CREAT | O_APPEND, 0755);
                    args[j] = args[j+1] = NULL;
                }
            }
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

int main(void)
{
    time_t now;
    srand((unsigned int) (time(&now)));

    char *args[MAX_ARGS];
    int bg;

    if(signal(SIGINT, sigHandler) == SIG_ERR){
        printf("Could not bind sig handler\n");
        exit(1);
    }

    while(1){
        bg = 0;
        int cnt = getcmd("\n>> ", args, &bg);

        if(cnt <= 0){
            continue;
        }
        
        //Increases execution time for syscalls
 /*       int w, rem;
        w =  rand() % 10;
        rem = sleep(w);
        while(rem != 0){
            rem = sleep(rem);
        }*/

        exec(args, bg);
    }
}