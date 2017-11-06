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

//Semaphore declaration for both sections
sem_t *mutexA;
sem_t *mutexB;

//Shared memmory and base address variables
int shm_fd;
void *base;

//Table data structure
struct table
{
    int num;
    char name[25];
};

//Initialize both sections with table numbers
void initTables(struct table *base)
{
    //Use sem_wait before entering critical section
    sem_wait(mutexA);
    sem_wait(mutexB);
    
    //Critical section

    for (int i = 0; i < 10; i++){
        (base + i)->num = i + 100;
        strcpy((base + i)->name, "");
    }

    for(int i = 0; i < 10; i++){
        (base + i + 10)->num = i + 200;
        strcpy((base + i + 10)->name, "");
    }
  
    //End of critical section

    sleep(rand() % 3);

    //release the mutexes using sem_post
    sem_post(mutexA);
    sem_post(mutexB);

    return;
}

//Initialize database without having to call init first when shared mem is empty
void populateTables(struct table *base)
{
    if(base->num == 0)
    {
        initTables(base);
    }
}

//Display reservation info
void printTableInfo(struct table *base)
{
    sem_wait(mutexA);
    sem_wait(mutexB);
    
    //Critical section

    printf("\nSECTION A\n\n");
    for (int i = 0; i < 10; i++){
        printf("Table %d: %s\n", (base + i)->num, (base + i)->name);
    }

    printf("\nSECTION B\n\n");
    for(int i = 0; i < 10; i++){
        printf("Table %d: %s\n", (base + i + 10)->num, (base + i + 10)->name);
    }

    //End of critical section

    sleep(rand() % 3);
    
    sem_post(mutexA);
    sem_post(mutexB);

    return; 
}

//Reserve a specific table number
void reserveSpecificTable(struct table *base, char *nameHld, char *section, int tableNo)
{
    switch (section[0])
    {
    case 'a':
    case 'A':

        sem_wait(mutexA);

        //Critical section

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
        
        //End

        sleep(rand() % 3);

        sem_post(mutexA);

        break;
    case 'b':
    case 'B':
        
        sem_wait(mutexB);
        
        //Critical section

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

        //End

        sleep(rand() % 3);

        sem_post(mutexB);

        break;
    default:
        printf("Invalid section\n");
        break;
    }
    return;
}

//Reserve any table in a specified section
void reserveSomeTable(struct table *base, char *nameHld, char *section)
{
    int idx = -1;
    int i;
    int tableFound;
    switch (section[0])
    {
    case 'a':
    case 'A':
        
        sem_wait(mutexA);
        tableFound = 0;
        
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

        sleep(rand() % 3);

        sem_post(mutexA);

        break;
    case 'b':
    case 'B':
        
        sem_wait(mutexB);
        tableFound = 0;
        
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

        sleep(rand() % 3);

        sem_post(mutexB);

        break;
    default:
        printf("Invalid section\n");
        break;
    }
}

//Processes commands from input
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

        sleep(rand() % 3);
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

    //Rewire file descriptor when passing test file as input
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
    
    //Open mutexes
    mutexA = sem_open(BUFF_MUTEX_A, O_CREAT, 0777, 1);
    mutexB = sem_open(BUFF_MUTEX_B, O_CREAT, 0777, 1);

    if(mutexA == SEM_FAILED || mutexB == SEM_FAILED)
    {
        printf("Sem open failed: %s\n", strerror(errno));
        exit(-1);
    }

    //Open shared mem
    shm_fd = shm_open(BUFF_SHM, O_CREAT | O_RDWR, 0777);

    if (shm_fd == -1)
    {
        printf("Shared memory failed: %s\n", strerror(errno));
        exit(-1);
    }

    //Truncate and set base addr
    ftruncate(shm_fd, memSize);
    base = mmap(NULL, memSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (base == MAP_FAILED)
    {
        printf("Map failed: %s\n", strerror(errno));
        shm_unlink(BUFF_SHM);
        exit(-1);
    }

    //Rand number generator
    time_t now;
    srand((unsigned int)(time(&now)));

    char cmd[100];
    int ret = 1;

    populateTables(base);

    while (ret)
    {
        printf("\n>>");
        fgets(cmd, sizeof(cmd), stdin);     //Use fgets instead of gets, doesn't segfault when passing no input
        cmd[sizeof(cmd) - 1] = '\0';        //Removes carriage return from input
        if (argc>1)
        {
            printf("Executing Command: %s\n", cmd);
        }
        ret = processCmd(cmd, base);
    }
    
    //close semaphores
    sem_close(mutexA);
    sem_close(mutexB);

    //reset stdin
    if (argc > 1)
    {
        dup2(fdstdin, 0);
    }

    //unmap shared mem
    munmap(base, memSize);
    close(shm_fd);

    return 0;
}