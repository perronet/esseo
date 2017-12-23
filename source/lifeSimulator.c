#include "lifeSimLib.h"
#include "life_sim_stats.h"
#include "signal.h"

#define MUTEX_P errno = 0; sops.sem_num=SEM_NUM_MUTEX;\
				sops.sem_op = -1; \
                semop(semid, &sops, 1); TEST_ERROR /*ACCESSING*/
    
#define MUTEX_V errno = 0; sops.sem_num=SEM_NUM_MUTEX;\
				sops.sem_op = 1; \
        		semop(semid, &sops, 1); TEST_ERROR /*RELEASING*/

unsigned int birth_death, sim_time, alrmcount = 0; //global
int semid, msgid;
shared_data * infoshared;
life_sim_stats stats;

//Machine state of the manager, determining the current state of the simulation
enum current_state {STARTING,RUNNING,FINISHED} state;

//Handles birth_death timer signal
void handle_sigalarm(int signal);
//Creates a new type, basing on current state of the simulation, trying to balance A and B processes
char new_individual_type(unsigned int a_pop, unsigned int b_pop);
//Generates a random genome, distributed from x to genes+x
unsigned long rnd_genome(unsigned long x, unsigned long genes);
//Returns a random uppercase character, assuming they are all contiguos in encoding
char rnd_char();
//Returns a string with an extra appended char
void append_newchar(char * dest, char * src);
//Creates a new individual by forking and executing the individual process
void create_individual(char type, char * name, unsigned long genome);
//Reads the given parameters from the config file and sets them up
void setup_params(unsigned int * init_people,unsigned long * genes,unsigned int * birth_death,unsigned int * sim_time);

int main(){

    //****************************************************************
    //SETTING UP SIMULATION
    //****************************************************************

    state = STARTING;
    unsigned int init_people; // initial population value
    unsigned long genes;//initial max value of genome
    birth_death;//tick interval of random killing and rebirth
    sim_time; // total duration of simulation

    setup_params(&init_people,&genes,&birth_death,&sim_time);

    int status, i, k;
    ind_data partner_1, partner_2;
    char nextType, nextName[MAX_NAME_LEN];

    init_stats(&stats);
   
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
    memid = shmget(SHM_KEY, 1, 0666 | IPC_CREAT);
    TEST_ERROR;
    shmctl ( memid , IPC_RMID , NULL ) ;
    TEST_ERROR;
#endif
    infoshared = get_shared_data();

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
    
	//Initialize array of b individuals in shared memory (no need for semaphore here)
	for(k=0; k<MAX_INIT_PEOPLE; k++){
		infoshared->alive_individuals[k] = 0;
	}
    //****************************************************************
    //FIRST INITIALIZATION OF INDIVIDUALS
    //****************************************************************
    srand(getpid() + time(NULL));//FIXME This works but it's weak and ugly, needs replacement

    for(i=0;i<init_people;i++){
    	nextType = new_individual_type(stats.total_population_a,stats.total_population_b);//We use the stats here and not the shared info because processes will sleep until we finish this. We could get inconsistent data.
        nextName[0] = rnd_char();

#if CM_DEBUG_BALANCED || CM_DEBUG_COUPLE //Balance A and B individuals
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
	sops.sem_num=SEM_NUM_MUTEX;

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
			LOG(LT_MANAGER_ACTIONS,"%d ######## MANAGER WAITING FIRST PID of %d and %d!\n",msgcount, partner_1.pid, partner_2.pid);
			waitpid(partner_1.pid, &status, 0);
			msgrcv(msgid, &msg, MSGBUF_LEN, partner_1.pid, 0);//wait for partner data (will only receive from B processes)
			msgcount++; 
			ind_data_cpy(&partner_2, &(msg.info));
			LOG(LT_MANAGER_ACTIONS,"%d ######## MANAGER WAITING SECOND PID of %d and %d!\n",msgcount, partner_1.pid, partner_2.pid);
			waitpid(partner_2.pid, &status, 0);
			LOG(LT_MANAGER_ACTIONS,"%d ######## MANAGER created COUPLE %d and %d!\n",msgcount, partner_1.pid, partner_2.pid);

			int gcdiv = gcd(partner_1.genome, partner_2.genome);
			nextType = new_individual_type(infoshared->current_pop_a,infoshared->current_pop_b);
			//nextType = 'A';
			append_newchar(nextName, partner_1.name);
			create_individual(nextType, nextName, rnd_genome(gcdiv, genes));

			nextType = new_individual_type(infoshared->current_pop_a,infoshared->current_pop_b);
			//nextType = 'B';
			append_newchar(nextName, partner_2.name);

			create_individual(nextType, nextName, rnd_genome(gcdiv, genes));

			stats.total_couples++;
		}
	}

	return 0;
}

//****************************************************************
//INDIVIDUALS CREATION FUNCTIONS
//****************************************************************

char new_individual_type(unsigned int a_pop, unsigned int b_pop){    
	float a_type_probability;

	if(b_pop == 0)
		a_type_probability = 0;
	else
		a_type_probability = (float)b_pop / (float)(b_pop + a_pop); // This way we a types have more chances to appear when they are fewer

	char new_type = rand()%100 <= a_type_probability * 100.0 ? 'A' : 'B';
	
	//LOG(LT_MANAGER_ACTIONS, "Requested ind type. Since a_pop =%u,b_pop=%u,a_type_probability=%f,result was %c\n",a_pop,b_pop,a_type_probability,new_type);
    
    return new_type; //random type
}

unsigned long rnd_genome(unsigned long x, unsigned long genes){
    return rand()%genes +x; //Random unsigned long from x to genes+x
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
    pid_t child_pid;

    //Let's block the alarm. Since we are creating a new individuals, data could be inconsistent if we killed someone in the meantime
	sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&my_mask, SIGALRM); 
	sigprocmask(SIG_BLOCK, &mask, NULL);

    switch(child_pid = fork()){
        case -1://Error occured
            TEST_ERROR;
            exit(EXIT_FAILURE);
            break; 
        case 0://Child process
            ;//This is necessary to make the compiler happy, since we cannot have declarations next to labels. A label can only be part of a statement and a declaration is not a statement
			sigprocmask(SIG_UNBLOCK, &mask, NULL);
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
            ;//This is necessary to make the compiler happy, since we cannot have declarations next to labels. A label can only be part of a statement and a declaration is not a statement
	       	ind_data new_individual;
        	new_individual.type = type;
        	strcpy(new_individual.name, name);
        	new_individual.genome = genome;
        	new_individual.pid = child_pid;
        	register_individual_in_stats(&stats, &new_individual);
    		insert_pid(infoshared->alive_individuals, child_pid);//no need for semaphore, only manager writes here and alarm is blocked

			sigprocmask(SIG_UNBLOCK, &mask, NULL);

            break;
        }
}

void setup_params(unsigned int * init_people,unsigned long * genes,unsigned int * birth_death,unsigned int * sim_time)
{
	FILE *config_file = NULL;

    *init_people = INIT_PEOPLE_DEFAULT; // initial population value
    *genes = GENES_DEFAULT;//initial max value of genome
    *birth_death = BIRTH_DEATH_DEFAULT;//tick interval of random killing and rebirth
    *sim_time = SIM_TIME_DEFAULT; // total duration of simulation
	
	// Opening the file to be read
	if ((config_file = fopen(CONFIG_FILE_NAME, "r")) == NULL) {
		LOG(LT_GENERIC_ERROR, "Error opening config file \"%s\"., MSG:%s\n", CONFIG_FILE_NAME, strerror(errno));
	}
	else
	{
		char options_buffer [INPUT_BUF_LEN];
		char values_buffer [INPUT_BUF_LEN];
		char * current_buffer = options_buffer;//We will swap betwen the two above buffers via this one

		int buf_ind = 0;//index of buffer
		char c;//current char

		while ((c = fgetc(config_file)) != EOF) 
		{
			if(c != '=' && c != '\n')
			{//store a token string inside the buffer
				current_buffer[buf_ind] = c;
				buf_ind ++;
			}
			else
			{//we have a token
				current_buffer[buf_ind] = '\0';//Manually set EOS
				buf_ind = 0;//reset index to restart reading

				if(current_buffer == values_buffer)
				{//We should have something in the options_buffer and in the values_buffer. Let's look.
					
					unsigned long value = string_to_ulong(values_buffer);// let's use the biggest type initially

					if(strcmp(options_buffer,INIT_PEOPLE_CONFIG_NAME) == 0)
					{
						LOG(LT_MANAGER_ACTIONS,"Read %s from file, has value %lu\n",INIT_PEOPLE_CONFIG_NAME,value);
						*init_people = value;
					}
					else if(strcmp(options_buffer,GENES_CONFIG_NAME)== 0)
					{
						LOG(LT_MANAGER_ACTIONS,"Read %s from file, has value %lu\n",GENES_CONFIG_NAME,value);
						*genes = value;
					}
					else if(strcmp(options_buffer,BIRTH_DEATH_CONFIG_NAME)== 0)
					{
						LOG(LT_MANAGER_ACTIONS,"Read %s from file, has value %lu\n",BIRTH_DEATH_CONFIG_NAME,value);
						*birth_death = value;
					}
					else if(strcmp(options_buffer,SIM_TIME_CONFIG_NAME)== 0)
					{
						LOG(LT_MANAGER_ACTIONS,"Read %s from file, has value %lu\n",SIM_TIME_CONFIG_NAME,value);
						*sim_time = value;
					}
					else
					{//invalid!
						LOG(LT_GENERIC_ERROR,"Invalid token '%s'\n",options_buffer);
					}

					current_buffer = options_buffer;//Let's get ready to read the next value
				}
				else
					current_buffer = values_buffer;//We just finished reading an option name. Now let's read the value
			}

		}
	}

#if CM_DEBUG_COUPLE
    *init_people = 2;
    LOG(LT_MANAGER_ACTIONS,"Setting init_people to 2 because CM_DEBUG_COUPLE is true\n");
#endif
	
	//Bad values checking
	if(*init_people < 2){
        LOG(LT_GENERIC_ERROR,"Warning: init_people should be a value greater than 1. Setting init_people to default value '%d'\n", INIT_PEOPLE_DEFAULT); 
        *init_people = INIT_PEOPLE_DEFAULT;
    }
    if(*init_people > MAX_INIT_PEOPLE){
        LOG(LT_GENERIC_ERROR,"Warning: init_people should be a value less than %u. Setting init_people to this value.\n", MAX_INIT_PEOPLE); 
        *init_people = MAX_INIT_PEOPLE;
    }
    if(*sim_time <= 2){
    	LOG(LT_GENERIC_ERROR,"Warning: sim_time should be a value higher or equal to 2. Setting sim_time to default value '%d'\n", SIM_TIME_DEFAULT);
        *sim_time = SIM_TIME_DEFAULT;
    }
    if(*birth_death >= *sim_time){
    	LOG(LT_GENERIC_ERROR,"Warning: birth_death should be a value lower or equal to sim_time. Setting birth_death to '1'\n");
        *birth_death = 1;
    }

	LOG(LT_SHIPPING,"********SIMULATION PARAMETERS********\n");
	LOG(LT_SHIPPING,"%s : %u\n",INIT_PEOPLE_CONFIG_NAME,*init_people);
	LOG(LT_SHIPPING,"%s : %u\n",BIRTH_DEATH_CONFIG_NAME,*birth_death);
	LOG(LT_SHIPPING,"%s : %lu\n",GENES_CONFIG_NAME,*genes);
	LOG(LT_SHIPPING,"%s : %u\n",SIM_TIME_CONFIG_NAME,*sim_time);
	LOG(LT_SHIPPING,"*************************************\n");

	if(config_file)
		fclose(config_file);

	//exit(EXIT_SUCCESS);//use it to test input only
}

//****************************************************************
//SIGNAL & MESSAGE HANDLING
//****************************************************************

void handle_sigalarm(int signal) {
    alrmcount++;							//alrmcount * birth_death is the elapsed time
    if(sim_time > alrmcount * birth_death){ //handle birth_death events (kill a child, create a new child, PRINT stats)
    	if(sim_time >= (alrmcount+1) * birth_death){ //the next alarm could arrive after sim_time is reached
    		alarm(birth_death); //Schedule another alarm
    		LOG(LT_ALARM,"ALARM #%d Total population A:%d B:%d Actual population A:%d B:%d\n", alrmcount, stats.total_population_a, stats.total_population_b, infoshared->current_pop_a, infoshared->current_pop_b); 
    		stats.total_killed++;

    	}else{
    		alarm(sim_time - alrmcount * birth_death);
    		LOG(LT_ALARM,"ALARM #%d Total population A:%d B:%d Actual population A:%d B:%d\n", alrmcount, stats.total_population_a, stats.total_population_b, infoshared->current_pop_a, infoshared->current_pop_b); 
    	}
    }else{ 
    	LOG(LT_ALARM,"\n#########################################\nSIMULATION ENDING...\n");
		LOG(LT_ALARM,"#########################################\n\n");

	    //****************************************************************
	    //CONCLUSION OF SIMULATION / PRINT STATISTICS
	    //****************************************************************
	    state = FINISHED;


	    pid_t pid;
	    int status;

	    MUTEX_P
	
		unsigned int kills = 0;
	    for(int i = 0; i < MAX_INIT_PEOPLE; i++)
	    {// Kill every B
	    	if(infoshared->alive_individuals[i] != 0)
	    	{
	       		LOG(LT_ALARM,"KILLING PID=%d", infoshared->alive_individuals[i]);
       			kills++;
				kill(infoshared->alive_individuals[i], SIGKILL); 
	    	}
	    }
        
        int alive = infoshared->current_pop_a + infoshared->current_pop_b - kills;

	    if(alive > 0)
	    	LOG(LT_ALARM,"%u individuals still alive\n", alive);

	    msgbuf msg;
	    while(msgrcv(msgid, &msg, MSGBUF_LEN, getpid(), IPC_NOWAIT) != -1 && errno!=EINTR)
        {//Eliminate any A process pending
	       	LOG(LT_ALARM,"RECEIVED MSG PID=%d\n", msg.info.pid);
			kill(msg.info.pid, SIGKILL);

	    	while(msgrcv(msgid, &msg, MSGBUF_LEN, msg.info.pid, IPC_NOWAIT) != -1 && errno!=EINTR)
    		{//Eliminate any B process pending
				kill(msg.info.pid, SIGKILL);
	    	}
        }

		LOG(LT_ALARM,"\n#########################################\nSIMULATION END!\n");
		LOG(LT_ALARM,"#########################################\n\n");
	    //TODO DEALLOCATE SHARED STUFF

       	MUTEX_V

	    while ((pid = wait(&status)) != -1) { 
	       // LOG(LT_ALARM,"Got info of child with PID=%d, status=0x%04X\n", pid, status);
	    } //"kill" all zombies!
	    if(errno == ECHILD) {
			LOG(LT_ALARM,"In PID=%6d, no more child processes\n", getpid());
    		LOG(LT_ALARM,"Total population A:%d B:%d Actual population A:%d B:%d\n", stats.total_population_a, stats.total_population_b, infoshared->current_pop_a, infoshared->current_pop_b); 

 			print_stats(&stats);

			exit(EXIT_SUCCESS);
		}else {
			TEST_ERROR;
			exit(EXIT_FAILURE);
		}
    }
}
