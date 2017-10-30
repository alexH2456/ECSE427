#define _SVID_SOURCE
#define _BSD_SOURCE
#define _XOPEN_SOURCE 500
#define _XOPEN_SORUCE 600
#define _XOPEN_SORUCE 600

#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define BUFF_SIZE 20
#define BUFF_SHM "/OS_BUFF"
#define BUFF_MUTEX_A "/OS_MUTEX_A"
#define BUFF_MUTEX_B "/OS_MUTEX_B"

//declaring semaphores names for local usage
sem_t *mutexA;
sem_t *mutexB;

//declaring the shared memory and base address
int shm_fd;
void *base;

//structure for indivdual table
struct table
{
    int num;
    char name[10];
};

void initTables(struct table *base)
{
    //capture both mutexes using sem_wait
    
    //initialise the tables with table numbers
    
    //perform a random sleep  
    sleep(rand() % 10);

    //release the mutexes using sem_post
    return;
}

void printTableInfo(struct table *base)
{
    //capture both mutexes using sem_wait
    
    //print the tables with table numbers and name
    
    //perform a random sleep  
    sleep(rand() % 10);
    
    //release the mutexes using sem_post
    return; 
}

void reserveSpecificTable(struct table *base, char *nameHld, char *section, int tableNo)
{
    switch (section[0])
    {
    case 'A':
        //capture mutex for section A
        
        //check if table number belongs to section specified
        //if not: print Invalid table number 
        
        //reserve table for the name specified
        //if cant reserve (already reserved by someone) : print "Cannot reserve table"
        
       // release mutex
        break;
    case 'B':
        //capture mutex for section B
        
        //check if table number belongs to section specified
        //if not: print Invalid table number 
        
        //reserve table for the name specified ie copy name to that struct
        //if cant reserve (already reserved by someone) : print "Cannot reserve table"
        
       // release mutex
       break;
    }
    return;
}

void reserveSomeTable(struct table *base, char *nameHld, char *section)
{
    int idx = -1;
    int i;
    switch (section[0])
    {
    case 'A':
        //capture mutex for section A
    
        //look for empty table and reserve it ie copy name to that struct

        //if no empty table print : Cannot find empty table


        //release mutex for section A
        break;
    case 'B':
        //capture mutex for section A
    
        //look for empty table and reserve it ie copy name to that struct

        //if no empty table print : Cannot find empty table


        //release mutex for section A
        break;
    }
}

int processCmd(char *cmd, struct table *base)
{
    char *token;
    char *nameHld;
    char *section;
    char *tableChar;
    int tableNo;
    token = strtok(cmd, " ");
    switch (token[0])
    {
    case 'r':
        nameHld = strtok(NULL, " ");
        section = strtok(NULL, " ");
        tableChar = strtok(NULL, " ");
        if (tableChar != NULL)
        {
            tableNo = atoi(tableChar);
            reserveSpecificTable(base, nameHld, section, tableNo);
        }
        else
        {
            reserveSomeTable(base, nameHld, section);
        }
        sleep(rand() % 10);
        break;
    case 's':
        printTableInfo(base);
        break;
    case 'i':
        initTables(base);
        break;
    case 'e':
        return 0;
    }
    return 1;
}

int main(int argc, char * argv[])
{
    int fdstdin;
    // file name specifed then rewire fd 0 to file 
    if(argc>1)
    {
        //store actual stdin before rewiring using dup in fdstdin
        
        //perform stdin rewiring as done in assign 1
       
    }
    //open mutex BUFF_MUTEX_A and BUFF_MUTEX_B with inital value 1 using sem_open
    mutexA = ;
    mutexB = ;

    //opening the shared memory buffer ie BUFF_SHM using shm open
    shm_fd = ;
    if (shm_fd == -1)
    {
        printf("prod: Shared memory failed: %s\n", strerror(errno));
        exit(1);
    }

    //configuring the size of the shared memory to sizeof(struct table) * BUFF_SIZE usinf ftruncate
    ftruncate(/*fill details*/)

    //map this shared memory to kernel space
    base = mmap(/*fill details*/);
    if (base == MAP_FAILED)
    {
        printf("prod: Map failed: %s\n", strerror(errno));
        // close and shm_unlink?
        exit(1);
    }

    //intialising random number generator
    time_t now;
    srand((unsigned int)(time(&now)));

    //array in which the user command is held
    char cmd[100];
    int cmdType;
    int ret = 1;
    while (ret)
    {
        printf("\n>>");
        gets(cmd);
        if(argc>1)
        {
            printf("Executing Command : %s\n",cmd);
        }
        ret = processCmd(cmd, base);
    }
    
    //close the semphores
    

    //reset the standard input
    if(argc>1)
    {
        //using dup2
    }

    //unmap the shared memory
    munmap(/*fill details*/);
    close(shm_fd);
    return 0;
}