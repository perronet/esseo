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
#include <sys/msg.h>

#define true 1
#define false 0
#define forever for(;;)

#define TEST_ERROR if (errno) {fprintf(stderr, \
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

//#define MSG_LEN 10 
#define MAX_NAME_LEN 300
#define MAX_AGENDA_LEN 300
#define INDIVIDUAL_FILE_NAME "./individual.out"

#define SHM_KEY 123456789 //TODO replace this with non-hardcoded key
#define SEMAPHORE_SET_KEY 578412563 //TODO replace this with non-hardcoded key
#define MSGQ_KEY 123456789 //TODO replace this with non-hardcoded key

enum semaphores_set_types{
	SEM_NUM_MUTEX, //Used to control mutual exclusion
	SEM_NUM_INIT, //Used to manage the syncronization of the initialization (all individuals have to wait the others before starting)
	SEM_NUM_MAX //Useful to get the number of semaphores
};

//****************************************************************
//COMPILE MODES
//****************************************************************

#define CM_DEBUG_COUPLE true //if true, only two individuals will be created, one for each type. Useful to debug relationship between individuals
#define CM_IPC_AUTOCLEAN true//if true, ipc objects are deallocated and reallocated at startup. Useful to debug and avoid getting messages of precedent runs

//****************************************************************
//LIBRARY FUNCTIONS
//****************************************************************

typedef char bool;
struct sembuf sops;
struct sigaction sa;
sigset_t  my_mask;

//This struct rapresent the data defining single individual
typedef struct data{
    char type; //Type of the individual, can be A or B.
    char name[MAX_NAME_LEN];//Name of the individual
    unsigned long genome;//Genome of the individual
    pid_t pid;//Pid of the individual
} ind_data;

//Contains all the public data
typedef struct shared_data{
    //unsigned long wr_id;//Next index where to write
	ind_data agenda[MAX_AGENDA_LEN];//The list of A processes will be published here
} shared_data;

//Message struct used by individuals to communicate
typedef struct msgbuf {
	long mtype;             
	char mtext;    
    ind_data info;
} msgbuf;

#define MSGBUF_LEN (sizeof(msgbuf) - sizeof(long))

//Converts the given string into an unsigned long
unsigned long string_to_ulong(char * c);

//Copies the content of the struct from the given source to the given dest
void ind_data_cpy(ind_data * dest, ind_data * src);

//Attaches to shared memory and gets the shared memory structs
shared_data * get_shared_data();

//Remove an individual from the public agenda. Return true if found, false otherwise
bool remove_from_agenda(ind_data * agenda, pid_t individual);
