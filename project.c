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
#define TEST_ERROR    if (errno) {dprintf(STDERR_FILENO, \
					  "%s:%d: PID=%5d: Error %d (%s)\n", \
					  __FILE__,			\
					  __LINE__,			\
					  getpid(),			\
					  errno,			\
					  strerror(errno));}

typedef struct shared_data{ //info about each type A process will be posted here
    unsigned long genome[4000]; //4000 is an arbitrary number, will fix
    char name[4000]; 
    pid_t pid[4000];
    int msgq_id[4000];
} shared_data;

unsigned int birth_death; //global

void handle_sigalarm(int signal) { 
    //handle birth_death events (kill a child, create a new child, print stats)
    alarm(birth_death); //schedule another alarm
    printf("ALARM!\n");
}

char rnd_char(){
    srand(getpid()); //getpid() as seed is better, time(NULL) could generate many child with the same seed
    return (rand()%2)+97; //random type
}

int main(){
    unsigned int init_people = 20, sim_time = 40; //we should use a config file and file descriptors to set these
    birth_death = 4;
    unsigned long genes = 100;

    int status, i, memid, pop_a = 0, pop_b = 0;
    pid_t pid, child_pid;
    shared_data * infoshared;
    struct sembuf sops;
    struct sigaction sa;
    
    if(init_people < 2){
        fprintf(stderr,"Warning: init_people should be a value greater than 1. Setting init_people to default value '2'");
        init_people = 2;
    }
    //Compile child code
    system("gcc type_A.c -o type_A.out"); 
    system("gcc type_B.c -o type_B.out");
    //Setup signal handler
	sigset_t  my_mask;
    sa.sa_handler = &handle_sigalarm; 
	sa.sa_flags = 0; 
	sigemptyset(&my_mask);        
	sa.sa_mask = my_mask;
    sigaction(14, &sa, NULL);
    //Create shared memory
    memid = shmget(IPC_PRIVATE, sizeof(*infoshared), 0666);
	TEST_ERROR;
	infoshared = shmat(memid, NULL, 0); //attach pointer
	TEST_ERROR;
    //char* args[] = {""}
    for(i=0;i<init_people;i++){
        switch(child_pid = fork()){
            case -1:
                TEST_ERROR;
                exit(EXIT_FAILURE);
                break;
            case 0:
                if(rnd_char() == 'a'){
                    //wait for all child process (use semaphore here!)
                    execve("./type_A.out", NULL, NULL);
                }else{
                    //wait for all child process (use the same semaphore here!)
                    execve("./type_B.out", NULL, NULL);
                }
                TEST_ERROR;
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }  
    //let all child process blocked on the semaphore start at the same time here with a semop, then:    
    alarm(birth_death);
    sleep(10); //just a test to trigger the alarm

    while (wait(&status) != -1) { } //"kill" all zombies!
    if(errno == ECHILD) {
		printf("In PID=%6d, no more child processes\n", getpid());
        printf("The population was A:%d, B:%d\n", pop_a, pop_b); //we should find a way to count these in the father process (maybe shared memory?)
		exit(EXIT_SUCCESS);
	}else {
		TEST_ERROR;
		exit(EXIT_FAILURE);
	}
        
	return 0;
}
