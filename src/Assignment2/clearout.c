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
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define BUFF_SHM "/OS_BUFF"
#define BUFF_MUTEX_A "/OS_MUTEX_A"
#define BUFF_MUTEX_B "/OS_MUTEX_B"

int main()
{   
    //unlink shared mem
    shm_unlink(BUFF_SHM);
   	
    //unlink semaphores
    sem_unlink(BUFF_MUTEX_A);
    sem_unlink(BUFF_MUTEX_B);
  
    printf("Cleared memory\n");

    return 0;
}
