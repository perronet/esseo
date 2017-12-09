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

#define true 1;
#define false 0;

#define TEST_ERROR    if (errno) {fprintf(stderr, \
					  "%s:%d: PID=%5d: Error %d (%s)\n", \
					  __FILE__,			\
					  __LINE__,			\
					  getpid(),			\
					  errno,			\
					  strerror(errno));}
#define IS_TYPE_A(type) (type == 'A')
#define IS_TYPE_B(type) (type == 'B')
#define CHECK_VALID_IND_TYPE(type) if(!IS_TYPE_A(type) && !IS_TYPE_B(type)) \
					  {fprintf(stderr, \
					  "%s:%d: PID=%5d: Error:%s '%c'\n", \
					  __FILE__,			\
					  __LINE__,			\
					  getpid(),			\
					  "Individual type has to be 'A' or 'B', found", \
					  type);			\
					  exit(0);			\
					}

#define MSG_LEN 120 
#define MAX_NAME_LEN 300
#define MAX_AGENDA_LEN 300
#define INDIVIDUAL_FILE_NAME "./individual.out"

typedef char bool;

//This struct rapresent the data defining single individual
typedef struct data{
    char type; //Type of the individual, can be A or B.
    char name[MAX_NAME_LEN];//Name of the individual
    unsigned long genome;//Genome of the individual
    pid_t pid;//Pid of the individual
} ind_data;

//Contains all the public data
typedef struct shared_data{
	ind_data agenda[MAX_AGENDA_LEN];//The list of A processes will be published here
    unsigned int pop_a, pop_b;//Total count of the population
} shared_data;

//Message struct used by individuals to communicate
typedef struct msgbuf {
	long mtype;             
	char mtext[MSG_LEN];    
} msgbuf;

//Converts the given string into an unsigned long
unsigned long string_to_ulong(char * c);

//Copies the content of the struct from the given source to the given dest
void ind_data_cpy(ind_data * dest, ind_data * src);