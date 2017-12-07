#include "my-lib.h"
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

    int status, i, memid;
    pid_t pid, child_pid;
    shared_data * infoshared;
    struct sembuf sops;
    struct sigaction sa;
    char * args[] = {""};
    
    if(init_people < 2){
        fprintf(stderr,"Warning: init_people should be a value greater than 1. Setting init_people to default value '2'");
        init_people = 2;
    }
    //Compile child code
    system("gcc type.c -o type.out"); 
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
    //Init
    infoshared->pop_a = 0;
    infoshared->pop_b = 0;
    for(i=0;i<init_people;i++){
        switch(child_pid = fork()){
            case -1:
                TEST_ERROR;
                exit(EXIT_FAILURE);
                break;
            case 0:
                if(rnd_char() == 'a'){
                    infoshared->pop_a++;//VA SEMAFORATO!
                    //wait for all child process (use semaphore here!)
                    execve("./type.out", args, NULL);
                }else{
                    infoshared->pop_b++;//VA SEMAFORATO!
                    //wait for all child process (use the same semaphore here!)
                    execve("./type.out", NULL, NULL);
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
        printf("The population was A:%d, B:%d\n", infoshared->pop_a, infoshared->pop_b);
		exit(EXIT_SUCCESS);
	}else {
		TEST_ERROR;
		exit(EXIT_FAILURE);
	}
        
	return 0;
}
