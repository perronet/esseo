#include "lifeSimLib.h"

unsigned int birth_death, sim_time, pop_a, pop_b, alrmcount = 0; //global
shared_data * shared_info;

//Machine state of the manager, determining the current state of the simulation
enum current_state {STARTING,RUNNING,FINISHED} state;

//Handles birth_death timer signal
void handle_sigalarm(int signal);
//Creates a new type, basing on the given probability of getting a type a. Probability is given from 0 to 1
char new_individual_type(float a_type_probability);
//Generates a random genome, distributed from x to genes+x
unsigned long rnd_genome(unsigned long x, unsigned long genes);
//Returns a random uppercase character, assuming they are all contiguos in encoding
char rnd_char();
//Returns a string with an extra appended char
void append_newchar(char * dest, char * src);
//Creates a new individual by forking and executing the individual process
void create_individual(char type, char * name, unsigned long genome);

int main(){

    //****************************************************************
    //SETTING UP SIMULATION
    //****************************************************************

    state = STARTING;
    unsigned int init_people = 20; // initial population value
    birth_death = 1;//tick interval of random killing and rebirth
    sim_time = 55; // total duration of simulation
    unsigned long genes = 100;//initial max value of genome


#if CM_DEBUG_COUPLE
    init_people = 2;
#endif

    int status, i, semid, msgid;
    ind_data partner_1, partner_2;
    char nextType, nextName[MAX_NAME_LEN];
    if(init_people < 2){
        LOG(LT_GENERIC_ERROR,"Warning: init_people should be a value greater than 1. Setting init_people to default value '2'"); 
        init_people = 2;
    }
    if(birth_death > sim_time){
    	LOG(LT_GENERIC_ERROR,"Warning: birth_death should be a value lower or equal to sim_time. Setting birth_deatg to default value '0'");
        birth_death = 0;
    }
	//***Init of signal handlers and mask
    sa.sa_handler = &handle_sigalarm; 
	sa.sa_flags = 0; 
	sigemptyset(&my_mask); 
	sa.sa_mask = my_mask; //Signals to be masked in handler
    //sigaddset(&my_mask, SIGINT);   
    //sigaddset(&my_mask, SIGTERM);    
    //sigaddset(&my_mask, SIGQUIT);
    //sigprocmask(SIG_BLOCK, &my_mask, NULL); //Set process mask so that this process ignores interrupt signals 
    sigaction(SIGALRM, &sa, NULL);
    
    //Testing signals (the process will ignore them!)
    //raise(SIGINT);
    //raise(SIGTERM);
    //raise(SIGQUIT);

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
#elif CM_DEBUG_BALANCED //Balance A and B individuals
		if(i%2)
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

	#if CM_NOALARM 
      birth_death = 0; 
    #endif
    if(birth_death > 0){ 
    	alarm(birth_death); //Will send sigalarm every birth_death seconds
 	}else{ //No child will be killed
 		birth_death = sim_time;
 		alarm(sim_time);
 	}

    msgbuf msg;
    
    int msgcount = 0;
	forever{	    
	    if(msgrcv(msgid, &msg, MSGBUF_LEN, getpid(), 0) != -1 && errno!=EINTR)//wait for response (will only receive from A processes)
		{
			msgcount++;
			ind_data_cpy(&partner_1, &(msg.info));
			waitpid(partner_1.pid, &status, 0);
			msgrcv(msgid, &msg, MSGBUF_LEN, partner_1.pid, 0);//wait for partner data (will only receive from B processes)
			msgcount++; 
			ind_data_cpy(&partner_2, &(msg.info));
			waitpid(partner_2.pid, &status, 0);
			LOG(LT_MANAGER_ACTIONS,"%d ######## magic happened for %d and %d!\n",msgcount, partner_1.pid, partner_2.pid);

			//Produce two new individuals //FIXME it generates errors because we are using msgcount counter to exit, need to use simulation time before exiting
			int gcdiv = gcd(partner_1.genome, partner_2.genome);
			//nextType = new_individual_type(.5f); //FIXME there shouldn't be a .5 fixed value
			nextType = 'A';
			append_newchar(nextName, partner_1.name);
			create_individual(nextType, nextName, rnd_genome(gcdiv, genes));

			//nextType = new_individual_type(.5f); //FIXME there shouldn't be a .5 fixed value
			nextType = 'B';
			append_newchar(nextName, partner_2.name);
			create_individual(nextType, nextName, rnd_genome(gcdiv, genes));
		}
	}

	return 0;
}

//****************************************************************
//INDIVIDUALS CREATION FUNCTIONS
//****************************************************************

char new_individual_type(float a_type_probability){    
    return rand()%100 <= a_type_probability * 100.0 ? 'A' : 'B'; //random type
}

unsigned long rnd_genome(unsigned long x, unsigned long genes){
    return rand()%genes+x; //Random unsigned long from x to genes+x
}

char rnd_char(){
    return (rand()%26)+ 'A'; //random name 
}


void append_newchar(char * dest, char * src){
	size_t len = strlen(src);
	if(len < MAX_NAME_LEN - 1){
		strcpy(dest, src);
      	dest[len] = rnd_char();
       	dest[len+1] = '\0';
    }else{
       	strcpy(dest, src);
       	LOG(LT_GENERIC_ERROR, "MAX NAME LENGTH REACHED\n"); 
    }	
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
    alrmcount++;							//alrmcount * birth_death is the elapsed time
    if(sim_time > alrmcount * birth_death){ //handle birth_death events (kill a child, create a new child, PRINT stats)
    	if(sim_time >= (alrmcount+1) * birth_death){ //the next alarm could arrive after sim_time is reached
    		alarm(birth_death); //Schedule another alarm
    		LOG(LT_MANAGER_ACTIONS,"ALARM!\n"); 
    	}else{
    		alarm(sim_time - alrmcount * birth_death);
    		LOG(LT_MANAGER_ACTIONS,"ALARM!\n"); 
    	}
    }else{ 
	    //****************************************************************
	    //CONCLUSION OF SIMULATION / PRINT STATISTICS
	    //****************************************************************
	    state = FINISHED;
	    pid_t pid;
	    int status;
	    LOG(LT_MANAGER_ACTIONS,"SIMULATION END!\n"); //send SIGUSR1 to all children, wait all children, deallocate everything, print stats, exit

	    while ((pid = wait(&status)) != -1) { 
	        LOG(LT_MANAGER_ACTIONS,"Got info of child with PID=%d, status=0x%04X\n", pid, status);
	    } //"kill" all zombies!
	    if(errno == ECHILD) {
			LOG(LT_MANAGER_ACTIONS,"In PID=%6d, no more child processes\n", getpid());
	        LOG(LT_MANAGER_ACTIONS,"The population was A:%d, B:%d\n", pop_a, pop_b); //TODO Maybe print other stats 
			exit(EXIT_SUCCESS);
		}else {
			TEST_ERROR;
			exit(EXIT_FAILURE);
		}
    }
}
