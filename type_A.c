#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "ipc-msg-lib.h"

typedef struct data{
    char type;
    char name[500]; //arbitrary number, will fix
    unsigned long genome;
} data;

//should add pointer to shared memory shared_data (we can get its id with shmget using IPC_PRIVATE)

char rnd_char(){
    srand(getpid()); //getpid() as seed is better, time(NULL) could generate many child with the same seed
    return (rand()%26)+65; //random name 
}

unsigned long rnd_genome(int x, unsigned long genes){
    srand(getpid());
    return rand()%((x+genes)+1-x)+x; 
}

int main(){ 
    int i;
    int msgq_id, num_bytes; //will use message queue
    long rcv_type;
	struct msgbuf my_msg;
    data info;
    info.type = 'a'; 
    info.name[0] = rnd_char();
    info.genome = rnd_genome(50, 100);
    printf("Hello! my PID is: %d, i'm type %c, my name is %c, my genome is %li\n", getpid(), info.type, info.name[0], info.genome);

    exit(EXIT_SUCCESS);
	return 0;
}
