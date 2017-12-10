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


//****************************************************************
//Use this file to test whatever the fuck you want 
//****************************************************************

#define MSG_LEN 120 
#define MAX_NAME_LEN 300
#define MAX_AGENDA_LEN 300

//This struct rapresent the data defining single individual
typedef struct data{
    char type; //Type of the individual, can be A or B.
    char name[MAX_NAME_LEN];//Name of the individual
    unsigned long genome;//Genome of the individual
    pid_t pid;//Pid of the individual
} ind_data;
typedef struct dataw{
    char type; //Type of the individual, can be A or B.
    //char name[MAX_NAME_LEN];//Name of the individual
    unsigned long genome;//Genome of the individual
    pid_t pid;//Pid of the individual
} ind_dataw;

//Contains all the public data
typedef struct shared_data{
    unsigned long wr_id;//Next index where to write
	ind_data agenda[MAX_AGENDA_LEN];//The list of A processes will be published here
} shared_data;

//Message struct used by individuals to communicate
typedef struct msgbuf {
	long mtype;             
	char mtext[MSG_LEN];    
    ind_data info;
} msgbuf;

//Message struct used by individuals to communicate
typedef struct msgbufw {
	long mtype;             
	char mtext[MSG_LEN];    
    //ind_data info;
} msgbufw;

int main()
{
	printf("ind_data %lu\n",sizeof(ind_data));
	printf("ind_dataw %lu\n",sizeof(ind_dataw));
	printf("msgbuf %lu\n",sizeof(msgbuf));
	printf("msgbufw %lu\n",sizeof(msgbufw));
}