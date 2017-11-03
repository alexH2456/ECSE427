/* 
* Alexander Harris - 260688155
* ESCSE 427 - Assignment 2
* Fall 2017
* Reservation System
*/

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
#define STD_IN FILENO_STDIN
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
    char name[25];
};

void initTables(struct table *base)
{
    //capture both mutexes using sem_wait
    sem_wait(mutexA);
    sem_wait(mutexB);
    
    //initialise the tables with table numbers
    for (int i = 0; i < 10; i++){
        (base + i)->num = i + 100;
        strcpy((base + i)->name, "");
    }

    for(int i = 0; i < 10; i++){
        (base + i + 10)->num = i + 200;
        strcpy((base + i + 10)->name, "");
    }

    //perform a random sleep  
    sleep(rand() % 5);

    //release the mutexes using sem_post
    sem_post(mutexA);
    sem_post(mutexB);

    return;
}

void printTableInfo(struct table *base)
{
    //capture both mutexes using sem_wait
    sem_wait(mutexA);
    sem_wait(mutexB);
    
    //print the tables with table numbers and name
    printf("\nSECTION A\n\n");
    for (int i = 0; i < 10; i++){
        printf("Table %d: %s\n", (base + i)->num, (base + i)->name);
    }

    printf("\nSECTION B\n\n");
    for(int i = 0; i < 10; i++){
        printf("Table %d: %s\n", (base + i + 10)->num, (base + i + 10)->name);
    }

    //perform a random sleep  
    sleep(rand() % 5);
    
    //release the mutexes using sem_post
    sem_post(mutexA);
    sem_post(mutexB);

    return; 
}

void reserveSpecificTable(struct table *base, char *nameHld, char *section, int tableNo)
{
    switch (section[0])
    {
    case 'a':
    case 'A':
        //capture mutex for section A
        sem_wait(mutexA);

        //check if table number belongs to section specified
        if (tableNo >= 100 && tableNo < 110)
        {
            if(!strcmp((base + tableNo - 100)->name, ""))
            {
                strcpy((base + tableNo - 100)->name, nameHld);
                printf("Table %d reserved\n", tableNo);
            }
            else
            {
                printf("Table unavailable\n");
            }
        }
        else
        {
            printf("Invalid table number\n");
        }
        
        sleep(rand() % 5);

        // release mutex
        sem_post(mutexA);

        break;
    case 'b':
    case 'B':
        //capture mutex for section B
        sem_wait(mutexB);
        
        //check if table number belongs to section specified
        if (tableNo >= 200 && tableNo < 210)
        {
            if(!strcmp((base + tableNo - 190)->name, ""))
            {
                strcpy((base + tableNo - 190)->name, nameHld);
                printf("Table %d reserved\n", tableNo);
            }
            else
            {
                printf("Table unavailable\n");
            }
        }
        else
        {
            printf("Invalid table number\n");
        }

        sleep(rand() % 5);

        // release mutex
        sem_post(mutexB);

        break;
    default:
        printf("Invalid section\n");
        break;
    }
    return;
}

void reserveSomeTable(struct table *base, char *nameHld, char *section)
{
    int idx = -1;
    int i;
    int tableFound;
    switch (section[0])
    {
    case 'a':
    case 'A':
        //capture mutex for section A
        sem_wait(mutexA);
        tableFound = 0;
        //look for empty table and reserve it ie copy name to that struct

        //if no empty table print : Cannot find empty table
        for(int i = 0; i < 10; i++){
            if(!strcmp((base + i)->name, ""))
            {
                strcpy((base + i)->name, nameHld);
                printf("Table %d reserved\n", (base + i)->num);
                tableFound = 1;
                break;
            }
        }
        if(!tableFound){
            printf("No tables available in section %s\n", section);
        }

        sleep(rand() % 5);

        //release mutex for section A
        sem_post(mutexA);

        break;
    case 'b':
    case 'B':
        //capture mutex for section B
        sem_wait(mutexB);
        tableFound = 0;
        //look for empty table and reserve it ie copy name to that struct

        //if no empty table print : Cannot find empty table
        for(int i = 0; i < 10; i++){
            if(!strcmp((base + i + 10)->name, ""))
            {
                strcpy((base + i + 10)->name, nameHld);
                printf("Table %d reserved\n", (base + i + 10)->num);
                tableFound = 1;
                break;
            }
        }
        if(!tableFound){
            printf("No tables available in section %s\n", section);
        }

        sleep(rand() % 5);

        //release mutex for section B
        sem_post(mutexB);

        break;
    default:
        printf("Invalid section\n");
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
    case 'R':
    case 'r':
        nameHld = strtok(NULL, " ");
        section = strtok(NULL, " ");
        tableChar = strtok(NULL, " ");

        if (tableChar != NULL && nameHld != NULL && section != NULL)
        {
            tableNo = atoi(tableChar);
            reserveSpecificTable(base, nameHld, section, tableNo);
        }
        else if (nameHld != NULL && section != NULL)
        {
            reserveSomeTable(base, nameHld, section);
        }
        else
        {
            printf("Please specify name and section\n");
        }

        sleep(rand() % 5);
        break;
    case 'S':
    case 's':
        printTableInfo(base);
        break;
    case 'I':
    case 'i':
        initTables(base);
        break;
    case 'E':
    case 'e':
        return 0;
    }
    return 1;
}

int main(int argc, char * argv[])
{
    int fdstdin;
    char* inputFile;
    int memSize = sizeof(struct table) * BUFF_SIZE;

    if (argc>1)
    {
        if(access(argv[1], R_OK) == 0)
        {
            inputFile = argv[1];
            fdstdin = dup(0);
            close(0);
            open(inputFile, O_RDONLY, 0777);
        }
        else
        {
            printf("Input file does not exist\n");
            exit(-1);
        }

    }
    //open mutex BUFF_MUTEX_A and BUFF_MUTEX_B with inital value 1 using sem_open
    mutexA = sem_open(BUFF_MUTEX_A, O_CREAT, 0777, 1);
    mutexB = sem_open(BUFF_MUTEX_B, O_CREAT, 0777, 1);

    if(mutexA == SEM_FAILED || mutexB == SEM_FAILED)
    {
        printf("Sem open failed: %s\n", strerror(errno));
        exit(-1);
    }

    //opening the shared memory buffer ie BUFF_SHM using shm open
    shm_fd = shm_open(BUFF_SHM, O_CREAT | O_RDWR, 0666);

    if (shm_fd == -1)
    {
        printf("Shared memory failed: %s\n", strerror(errno));
        exit(-1);
    }

    //configuring the size of the shared memory to sizeof(struct table) * BUFF_SIZE usinf ftruncate
    ftruncate(shm_fd, memSize);

    //map this shared memory to kernel space
    base = mmap(NULL, memSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (base == MAP_FAILED)
    {
        printf("Map failed: %s\n", strerror(errno));
        shm_unlink(BUFF_SHM);
        exit(-1);
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
        fgets(cmd, sizeof(cmd), stdin);
        cmd[sizeof(cmd) - 1] = '\0';
        if(argc>1)
        {
            printf("Executing Command : %s\n", cmd);
        }
        ret = processCmd(cmd, base);
    }
    
    //close the semphores
    sem_close(mutexA);
    sem_close(mutexB);

    //reset the standard input
    if (argc>1)
    {
        dup2(fdstdin, 0);
    }

    //unmap the shared memory
    munmap(base, memSize);
    close(shm_fd);
    return 0;
}