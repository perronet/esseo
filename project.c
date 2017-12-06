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

typedef struct shared_data{
    char type;
    char name[500]; //arbitrary number, will fix
    unsigned long genome;
} shared_data;

void handle_sigalarm(int signal) {
    //handle birth_death events (kill a child, create a new child, print stats)
    //alarm(birth_death); //schedule another alarm
    printf("ALARM!\n");
}

int main(){
    unsigned int init_people = 20, birth_death = 4, sim_time = 40; //we should use a config file and file descriptors to set these
    unsigned long genes = 100;
    
    int status, i;
    pid_t pid, child_pid;
    //shared_data * info;
    struct sembuf sops;
    struct sigaction sa;

    //Setup signal handler
	sigset_t  my_mask;
    sa.sa_handler = &handle_sigalarm; 
	sa.sa_flags = 0; 
	sigemptyset(&my_mask);        
	sa.sa_mask = my_mask;
    sigaction(14, &sa, NULL);

    system("gcc child.c -o child.out"); //compile child code
    if(init_people < 2){
        fprintf(stderr,"Warning: init_people should be a value greater than 1. Setting init_people to default value '2'");
        init_people = 2;
    }
    for(i=0;i<init_people;i++){
        switch(child_pid = fork()){
            case -1:
                TEST_ERROR;
                exit(EXIT_FAILURE);
                break;
            case 0:
                //wait for all child process (use semaphore!)
                execve("./child", NULL, NULL); //exec child code
                TEST_ERROR;
                exit(EXIT_FAILURE);
                break;
            default:
                break;
        }
    }  
    //let all child process blocked on the semaphore start at the same time here with a semaphore, then:
    alarm(birth_death);
    sleep(10); //just a test to trigger the alarm

    while (wait(&status) != -1) { }
    if(errno == ECHILD) {
		printf("In PID=%6d, no more child processes\n", getpid());
		exit(EXIT_SUCCESS);
	}else {
		TEST_ERROR;
		exit(EXIT_FAILURE);
	}
        
	return 0;
}
