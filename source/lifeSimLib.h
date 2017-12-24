#ifndef LIFE_SIM_LIB_H//include guard
#define LIFE_SIM_LIB_H


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

//****************************************************************
//COMPILE MODES
//****************************************************************

#define CM_DEBUG_COUPLE false //if true, only two individuals will be created, one for each type. Useful to debug relationship between individuals
#define CM_SAY_ALWAYS_YES false //if true, individuals will always contact/accept any other one. 
#define CM_DEBUG_BALANCED true //if true, individuals A and B will be balanced
#define CM_IPC_AUTOCLEAN true//if true, ipc objects are deallocated and reallocated at startup. Useful to debug and avoid getting messages of previous runs
#define CM_NOALARM false //if true, lifeSimulator never sends ALARM signals
#define CM_SLOW_MO false //if true, some carefully placed sleeps will slow down the execution

//****************************************************************
//LOG TYPES
//****************************************************************
//-Use LOG_ENABLED to set the activation of a log
//-Use ERRLOG_ENABLED to set the activation of an error log

#define LT_INDIVIDUALS_ACTIONS LOG_ENABLED(false)
#define LT_MANAGER_ACTIONS LOG_ENABLED(true)
#define LT_AGENDA_STATUS LOG_ENABLED(false)
#define LT_INDIVIDUALS_ADAPTATION LOG_ENABLED(false)
#define LT_ALARM LOG_ENABLED(true)

#define LT_SHIPPING LOG_ENABLED(true)//All the output of the shipping version
 
#define LT_GENERIC_ERROR ERRLOG_ENABLED(true)

//****************************************************************
//LOG SYSTEM IMPLEMENTATION
//****************************************************************
//Below are the conditional log system macros

#define LOG_ENABLED(IS_ENABLED) LOG##IS_ENABLED
#define ERRLOG_ENABLED(IS_ENABLED) ERRLOG##IS_ENABLED

#define LOGfalse(...)//do nothing
#define LOGtrue(...) printf (__VA_ARGS__);//actually print
#define ERRLOGfalse(...)//do nothing
#define ERRLOGtrue(...) fprintf(stderr,__VA_ARGS__);//actually print
 
#define LOG(LOG_TYPE,...) LOG_TYPE(__VA_ARGS__)//Call this function to log! 

//****************************************************************
//USEFUL MACROS 
//****************************************************************

#define TEST_ERROR if (errno) {LOG(LT_GENERIC_ERROR,\
          "%s:%d: PID=%5d: Error %d (%s)\n", \
           __FILE__,      \
           __LINE__,      \
           getpid(),      \
           errno,      \
           strerror(errno));}
#define IS_TYPE_A(type) (type == 'A')
#define IS_TYPE_B(type) (type == 'B')
#define CHECK_VALID_IND_TYPE(type) if(!IS_TYPE_A(type) && !IS_TYPE_B(type)) {LOG(LT_GENERIC_ERROR, \
            "%s:%d: PID=%5d: Error:%s '%c'\n", \
            __FILE__,      \
            __LINE__,      \
            getpid(),      \
            "Individual type has to be 'A' or 'B', found", \
            type);      \
            exit(0);      \
          }

#define LOG_INDIVIDUAL(LOG_TYPE, INDIVIDUAL) LOG(LOG_TYPE, "*        type   : %-21c*\n*        name   : %-21s*\n*        genome : %-21lu*\n*        pid    : %-21d*\n",INDIVIDUAL.type,INDIVIDUAL.name,INDIVIDUAL.genome,INDIVIDUAL.pid);

#define MAX_NAME_LEN 500
#define MAX_AGENDA_LEN MAX_INIT_PEOPLE

//below are the defines for files
#define INDIVIDUAL_FILE_NAME "./individual.out"
#define CONFIG_FILE_NAME "sim_config"

#define INIT_PEOPLE_CONFIG_NAME "init_people"
#define INIT_PEOPLE_DEFAULT 20
#define MAX_INIT_PEOPLE 1000
#define GENES_CONFIG_NAME "genes"
#define GENES_DEFAULT 100
#define BIRTH_DEATH_CONFIG_NAME "birth_death"
#define BIRTH_DEATH_DEFAULT 1 //seconds
#define SIM_TIME_CONFIG_NAME "sim_time"
#define SIM_TIME_DEFAULT 60 //seconds
#define INPUT_BUF_LEN 100//max length of a config name / value

#define SHM_KEY 123456789 //we'll keep them hardcoded, it always worked
#define SEMAPHORE_SET_KEY 578412563 
#define MSGQ_KEY 123456789 

#define SLOW_MO_SLEEP_TIME 1 //Duration of sleeps in CM_SLOW_MO mode

enum semaphores_set_types{
	SEM_NUM_MUTEX, //Used to control mutual exclusion
	SEM_NUM_INIT, //Used to manage the syncronization of the initialization (all individuals have to wait the others before starting)
	SEM_NUM_MAX //Useful to get the number of semaphores
};

//****************************************************************
//STRUCTURES
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
	ind_data agenda[MAX_AGENDA_LEN];//The list of A processes will be published here. Invalid Type = empty cell
    pid_t alive_individuals[MAX_INIT_PEOPLE]; //The list of B processes will appear here. 0 = empty 
    unsigned int current_pop_a;
    unsigned int current_pop_b;
} shared_data;

//Message struct used by individuals to communicate
typedef struct msgbuf {
	long mtype;             
	char mtext;    
    ind_data info;
} msgbuf;

#define MSGBUF_LEN (sizeof(msgbuf) - sizeof(long))

//****************************************************************
//LIBRARY FUNCTIONS
//****************************************************************

//Converts the given string into an unsigned long
unsigned long string_to_ulong(char * c);

//Calculates greatest common divisor
unsigned long gcd(unsigned long a, unsigned long b);

//Copies the content of the struct from the given source to the given dest
void ind_data_cpy(ind_data * dest, ind_data * src);

//Attaches to shared memory and gets the shared memory structs
shared_data * get_shared_data();

//Remove an individual from the public agenda. Return true if found, false otherwise
bool remove_from_agenda(ind_data * agenda, pid_t individual);

//Prints the current occupied positions in the agenda. You should do this only in MUTEX to have continuos print
void print_agenda(ind_data * agenda);

//Insert integer in the first slot that equals 0
void insert_pid(pid_t * array, pid_t pid);

//Set slot that equals pid to 0
bool remove_pid(pid_t * array, pid_t pid);

#endif