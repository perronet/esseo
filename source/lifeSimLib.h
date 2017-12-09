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

#define CHECK_VALID_IND_TYPE(type) if(type != 'A' && type != 'B') \
					  {fprintf(stderr, \
					  "%s:%d: PID=%5d: Error:%s '%c'\n", \
					  __FILE__,			\
					  __LINE__,			\
					  getpid(),			\
					  "Individual type has to be 'A' or 'B', found", \
					  type);			\
					  exit(0);			\
					}
/*
#define ULONG_TO_STRING_DECLARE (n, string_name) char string_name [50];\//should be long enough, maybe we can compute the actual value with sizeof(long)
            			int cacca = sprintf (string_name, "%lu", n);\
            			*/

#define MSG_LEN 120 
#define MAX_NAME_LEN 300
#define INDIVIDUAL_FILE_NAME "./individual.out"

//THE BELOW STRUCT IS WRONG, PLUS WE WANT AN ARRAY OF STRUCTS, not a struct with arrays
typedef struct shared_data{ //info about each type A process will be posted here
    unsigned long genome[4000]; //4000 is an arbitrary number, will fix
    char name[4000]; 
    pid_t pid[4000];
    int msgq_id[4000];
    unsigned int pop_a, pop_b;
} shared_data;

typedef struct data{
    char type;
    char name[MAX_NAME_LEN]; //arbitrary number, will fix
    unsigned long genome;
} ind_data;

typedef struct msgbuf {
	long mtype;             
	char mtext[MSG_LEN];    
} msgbuf;
