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
#define TEST_ERROR    if (errno) {fprintf(stderr, \
					  "%s:%d: PID=%5d: Error %d (%s)\n", \
					  __FILE__,			\
					  __LINE__,			\
					  getpid(),			\
					  errno,			\
					  strerror(errno));}
#define MSG_LEN 120

typedef struct shared_data{ //info about each type A process will be posted here
    unsigned long genome[4000]; //4000 is an arbitrary number, will fix
    char name[4000]; 
    pid_t pid[4000];
    int msgq_id[4000];
    unsigned int pop_a, pop_b;
} shared_data;

typedef struct data{
    char type;
    char name[500]; //arbitrary number, will fix
    unsigned long genome;
} data;

typedef struct msgbuf {
	long mtype;             
	char mtext[MSG_LEN];    
} msgbuf;
