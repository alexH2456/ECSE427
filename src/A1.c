/* 
* Alexander Harris - 260688155
* ESCSE 427 - Assignment 1
* Fall 2017
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

#define MAX_ARGS 20

struct job *head_job = NULL;
struct job *tail_job = NULL;

struct job {   //Linked list object for jobs function
    int number; 
    int pid;
    char* job;
    struct job *next;
};

void addToJobList(char* args, int *pid)     //Adds a process to the job linked list
{
    struct job *job = malloc(sizeof(struct job));

    if (head_job == NULL) {
        job->number = 1;
        job->pid = *pid;
        job->job = args;
        job->next = NULL;
        head_job = job;
        tail_job = head_job;
    }
    else {

        job->number = tail_job->number + 1;
        job->pid = *pid;
        job->job = args;
        job->next = NULL;
        tail_job->next = job;
        tail_job = job;
        
    }
}

int findJob(int number)     //Finds a specified process number in the list and returns the pid
{
    struct job *running = head_job;

    while(running != NULL){
        if(running->number == number){

            return running->pid;
        }

        running = running->next;
    }
    return -1;
}

int getcmd(char *prompt, char *args[], int *background)     //Parses shell input for commands and arguments
{
    int length, i = 0;
    char *token, *loc;
    char *line = NULL;
    size_t linecap = 0;

    //Resets args array
    for (int j = 0; j < MAX_ARGS; j++){
        args[j] = NULL;
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

void sigHandler()   //Deals with CTRL+C and CTRL+Z interrupts
{
    signal(SIGINT, sigHandler);
    fflush(stdout);
    exit(-1);
}

void slowProcess(int sec)   //Slows process execution for testing purposes
{
    time_t now;
    srand((unsigned int) (time(&now)));

    int w, rem;
    w =  rand() % sec;
    rem = sleep(w);
    while(rem != 0){
        rem = sleep(rem);
    }
}

void exec(char* args[], int bg)     //Method for execution of all built-in and other functions
{
    int pid;
    
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
                fprintf((stderr), "Memory allocation failed\n");
            }
        }
    }

    //exit function
    if(strcmp(args[0], "exit") == 0){
        exit(0);
    }

    //cd function
    else if(strcmp(args[0], "cd") == 0){
        int dir = chdir(args[1]);
       
        if(dir != 0){
            printf("Invalid directory.\n");
        }
    }

    //jobs function
    else if(strcmp(args[0], "jobs") == 0){ 
       
        struct job *previous;
        struct job *current = head_job;

        while(current != NULL){
            if(waitpid(current->pid, NULL, WNOHANG) == 0){
                
                printf("<%d> Running: %s Pid: %d\n", current->number, current->job, current->pid);
                
                previous = current;
                current = current->next;
            }
            else{
               
                printf("<%d> Finished: %s Pid: %d\n", current->number, current->job, current->pid);
               
                head_job = current->next;
                current = head_job;
            }
        }
    }

    //fg function (not working)
    else if(strcmp(args[0], "fg") == 0){
       
        if(args[1] != NULL){
            int jobNumber = atoi(args[1]);
            pid = findJob(jobNumber);
        }
        if(pid == -1){
            printf("Job does not exist\n");
        }
    }

    //execvp for all other commands
    else{
        pid = fork();

        if(pid == 0){
               
            signal(SIGINT, sigHandler);     //CTRL+C kills child process

            //Output redirection
            for(int j = 0; args[j]; j++){

                char* redirect = args[j+1];

                if(strcmp(args[j], ">") == 0){     //Overwrite
                    close(1);
                    open(redirect, O_WRONLY | O_CREAT | O_TRUNC, 0755);
                    
                    args[j] = args[j+1] = NULL;
                }
                else if(strcmp(args[j], ">>") == 0){    //Append
                    close(1);
                    open(redirect, O_WRONLY | O_CREAT | O_APPEND, 0755);
                    
                    args[j] = args[j+1] = NULL;
                }
            }
            
            //slowProcess(10);
            execvp(args[0], args);
        }
        else if(pid > 0){

            if(bg == 0){
                waitpid(pid, NULL, WUNTRACED);
            }
            else{
                addToJobList(args[0], &pid);
            }
        }
        else{
            perror("Fork failed.\n");
            exit(-1);
        }
    }
}

int main(void)
{
    char *args[MAX_ARGS];
    int bg;

    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    while(1){
        bg = 0;
        int cnt = getcmd("\n>> ", args, &bg);

        if(cnt <= 0){ 
            continue;
        }

        exec(args, bg);
    }
}