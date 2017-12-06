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

typedef struct shared_data{
    char type;
    char name[500]; //arbitrary number, will fix
    unsigned long genome;
} shared_data;

char rnd_char(int a){
    char r;
    srand(getpid()); //getpid() as seed is better, time(NULL) could generate many child with the same seed
    if(!a){ 
        r = (rand()%2)+97;  //random type
    }else{      
        r = (rand()%26)+65; //random name
    }
    return r;
}

unsigned long rnd_genome(int x, unsigned long genes){
    srand(getpid());
    return rand()%((x+genes)+1-x)+x; 
}

int main(){ 
    int i;
    shared_data info;
    info.type = rnd_char(0); //it's just a test, we'll use a shared memory segment in project.c to set these
    info.name[0] = rnd_char(1);
    info.genome = rnd_genome(50, 100);
    printf("Hello! my PID is: %d, i'm type %c, my name is %c, my genome is %li\n", getpid(), info.type, info.name[0], info.genome);

    exit(EXIT_SUCCESS);
	return 0;
}
