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

int rnd_char(int a){
    int r = 0;
    srand(getpid());
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

void handle_sigalarm(int signal) {
    //handle end of simulation
}

int main(){
    unsigned int init_people = 20, birth_death = 4, sim_time = 40; //we should use a config file and file descriptors to set these
    unsigned long genes = 100;

    int status, i;
    pid_t pid, child_pid;
    shared_data * info;
    struct sembuf sops;
    struct sigaction sa;

    //Setup signal handler
	sigset_t  my_mask;
    sa.sa_handler = &handle_sigalarm; 
	sa.sa_flags = 0; 
	sigemptyset(&my_mask);        
	sa.sa_mask = my_mask;
    sigaction(14, &sa, NULL);

    if(init_people < 2){
        fprintf(stderr,"Warning: init_people should be a value greater than 1. Setting init_people to default value '2'");
        init_people = 2;
    }
    alarm(sim_time);
    //...        

	return 0;
}
