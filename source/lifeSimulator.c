#include "lifeSimLib.h"

unsigned int birth_death, pop_a, pop_b; //global
shared_data * shared_info;

//Machine state of the manager, determining the current state of the simulation
enum current_state {STARTING,RUNNING,FINISHED} state;

//Handles birth_death timer signal
void handle_sigalarm(int signal);
//Creates a new type, basing on the given probability of getting a type a. Probability is given from 0 to 1
char new_individual_type(float a_type_probability);
//Generates a random genome, distributed from x to genes+x
unsigned long rnd_genome(int x, unsigned long genes);
//Returns a random uppercase character, assuming they are all contiguos in encoding
char rnd_char();
//Creates a new individual by forking and executing the individual process
void create_individual(char type, char * name, unsigned long genome);

int main(){

    //****************************************************************
    //SETTING UP SIMULATION
    //****************************************************************

    state = STARTING;
    unsigned int init_people = 20, // initial population value
    				sim_time = 40; // total duration of simulation
    birth_death = 4;//tick interval of random killing and rebirth
    unsigned long genes = 100;//initial max value of genome

#if CM_DEBUG_COUPLE
    init_people = 2;
#endif

    int status, i, semid, msgid;
    pid_t pid;
    char nextType, nextName[MAX_NAME_LEN];
    
    if(init_people < 2){
        fprintf(stderr,"Warning: init_people should be a value greater than 1. Setting init_people to default value '2'");
        init_people = 2;
    }

	//***Init of signal handlers and mask
    sa.sa_handler = &handle_sigalarm; 
	sa.sa_flags = 0; 
	sigemptyset(&my_mask); 
    sigaddset(&my_mask, SIGINT);   
    sigaddset(&my_mask, SIGTERM);    
    sigaddset(&my_mask, SIGQUIT);
    sigprocmask(SIG_BLOCK, &my_mask, NULL); //Set process mask so that this process ignores interrupt signals 
    sa.sa_mask = my_mask; //Signals to be masked in handler (redundant, same as process mask)
    sigaction(SIGALRM, &sa, NULL);
    
    //Testing signals (the process will ignore them!)
    raise(SIGINT);
    raise(SIGTERM);
    raise(SIGQUIT);

	//***Init of shared memory
#if CM_IPC_AUTOCLEAN//deallocate and re allocate shared memory
    int memid;
    memid = shmget(SHM_KEY, sizeof(shared_data), 0666 | IPC_CREAT);
    TEST_ERROR;
    shmctl ( memid , IPC_RMID , NULL ) ;
    TEST_ERROR;
#endif
    shared_data * infoshared = get_shared_data();

	//***Init of semaphores
    semid = semget(SEMAPHORE_SET_KEY, SEM_NUM_MAX, 0666 | IPC_CREAT); //Array of 2 semaphores
	TEST_ERROR;
#if CM_IPC_AUTOCLEAN//deallocate and re allocate semaphores
	semctl(semid, 0, IPC_RMID);    
	TEST_ERROR;
    semid = semget(SEMAPHORE_SET_KEY, SEM_NUM_MAX, 0666 | IPC_CREAT); //Array of 2 semaphores
	TEST_ERROR;
#endif

	semctl(semid, SEM_NUM_INIT, SETVAL, init_people+1);//Sem init to syncronize the start of the individuals, initialized to init_people+1
	TEST_ERROR;
    semctl(semid, SEM_NUM_MUTEX, SETVAL, 1);//Sem mutex to control access to the shared memory, initialized to 1
	TEST_ERROR;

	//***Init of message queues
	msgid = msgget(MSGQ_KEY, 0666 | IPC_CREAT);
	TEST_ERROR;
#if CM_IPC_AUTOCLEAN//deallocate and re allocate the queue to avoid messages from precedent runs
    msgctl ( msgid , IPC_RMID , NULL ) ;//Empty the queue if already present
	TEST_ERROR;
    msgid = msgget(MSGQ_KEY, 0666 | IPC_CREAT);
	TEST_ERROR;
#endif
    
    //****************************************************************
    //FIRST INITIALIZATION OF INDIVIDUALS
    //****************************************************************

    pop_a = 0;
    pop_b = 0;
    srand(getpid() + time(NULL) + pop_a + pop_b);//FIXME This works but it's weak and ugly, needs replacement

    for(i=0;i<init_people;i++){
    	nextType = new_individual_type(.5f); //FIXME there shouldn't be a .5 fixed value
        nextName[0] = rnd_char();

#if CM_DEBUG_COUPLE //Only two individuals will be created, one for each type. This is a debug mode
		if(i)
			nextType = 'B';
			else        
			nextType = 'A';
#endif

        create_individual(nextType,nextName,rnd_genome(2, genes));//Only the father will return from this call
    } 
    
	sops.sem_num = SEM_NUM_INIT;
	sops.sem_flg = 0; 
    sops.sem_op = -1;
	semop(semid, &sops, 1);
    sops.sem_op = 0;
	semop(semid, &sops, 1); //Let's wait for the other processes to decrement the semaphore
                            //All individuals can start simultaneously now 
    //****************************************************************
    //SIMULATION IS RUNNING
    //**************************************************************** 

    state = RUNNING; 
    alarm(birth_death); //Will send sigalarm every birth_death seconds

    forever{
	    msgbuf msg;
	    if(msgrcv(msgid, &msg, MSGBUF_LEN, getpid(), 0) != -1 && errno!=EINTR)//wait for response
		{
	    	printf("magic happened for %d!\n",msg.info.pid);
	    	//kill(-1,SIGKILL);
	    	exit(EXIT_SUCCESS);
		}
	}
    
    //sleep(2); //just a test to trigger the alarm

    //****************************************************************
    //CONCLUSION OF SIMULATION / PRINT STATISTICS
    //****************************************************************

    state = FINISHED;//this could be moved to the handler of the end of simulation

    while ((pid = wait(&status)) != -1) {
        printf("Got info of child with PID=%d, status=0x%04X\n", pid, status);
    } //"kill" all zombies!
    if(errno == ECHILD) {
		printf("In PID=%6d, no more child processes\n", getpid());
        printf("The population was A:%d, B:%d\n", pop_a, pop_b);
		exit(EXIT_SUCCESS);
	}else {
		TEST_ERROR;
		exit(EXIT_FAILURE);
	}
	
	semctl(semid, 0, IPC_RMID);    
	return 0;
}

//****************************************************************
//INDIVIDUALS CREATION FUNCTIONS
//****************************************************************

char new_individual_type(float a_type_probability){    
    return rand()%100 <= a_type_probability * 100.0 ? 'A' : 'B'; //random type
}

unsigned long rnd_genome(int x, unsigned long genes){
    return rand()%genes+x; //Random unsigned long from x to genes+x
}

char rnd_char(){
    return (rand()%26)+ 'A'; //random name 
}

void create_individual(char type, char * name, unsigned long genome)
{
    CHECK_VALID_IND_TYPE(type)

    if(type == 'A')
        pop_a++;
    else
        pop_b++;

    switch(fork()){
        case -1://Error occured
            TEST_ERROR;
            exit(EXIT_FAILURE);
            break; 
        case 0://Child process
            ;//This is necessary to make the compiler happy, since we cannot have declarations next to labels. A label can only be part of a statement and a declaration is not a statement
            char genome_arg[50];
            sprintf(genome_arg,"%lu",genome);
            char wait_before_starting[1];
            wait_before_starting[0] = state == STARTING;//At the beginning of the simulation, individuals have to wait before starting to execute their behaviour
            char * argv[] = {INDIVIDUAL_FILE_NAME,&type,name,genome_arg, wait_before_starting, NULL};//File name goes first and NULL is last to respect standard
            execve(INDIVIDUAL_FILE_NAME,argv,NULL);
            TEST_ERROR;
            exit(EXIT_FAILURE);
            break;
        default://Father process
            break;
        }
}

//****************************************************************
//SIGNAL & MESSAGE HANDLING
//****************************************************************

void handle_sigalarm(int signal) { 
    //handle birth_death events (kill a child, create a new child, print stats)
    alarm(birth_death); //Schedule another alarm
    printf("ALARM!\n");
}
