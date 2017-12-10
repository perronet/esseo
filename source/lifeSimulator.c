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

    int status, i, semid;
    pid_t pid;
    char nextType, nextName[MAX_NAME_LEN];
    
    if(init_people < 2){
        fprintf(stderr,"Warning: init_people should be a value greater than 1. Setting init_people to default value '2'");
        init_people = 2;
    }

    //Setup signal handler
	sigset_t  my_mask;
    sa.sa_handler = &handle_sigalarm; 
	sa.sa_flags = 0; 
	sigemptyset(&my_mask);        
	sa.sa_mask = my_mask;
    sigaction(14, &sa, NULL);
    //Create shared memory
    shared_data * infoshared = get_shared_data();
    //Initialize semaphore
    semid = semget(123456789, 2, 0666 | IPC_CREAT); //Array of 2 semaphores FIXME I USED A HARDCODED KEY
	TEST_ERROR;
	semctl(semid, 0, SETVAL, 0);//Sem 0 to syncronize the start of the individuals, initialized to 0
    semctl(semid, 1, SETVAL, 1);//Sem 1 (mutex) to control access to the shared memory, initialized to 1
	TEST_ERROR;
	sops.sem_num = 0;//check the 0-th semaphore
	sops.sem_flg = 0; 
    
    //****************************************************************
    //FIRST INITIALIZATION OF INDIVIDUALS
    //****************************************************************

    pop_a = 0;
    pop_b = 0;
    for(i=0;i<init_people;i++){
    	nextType = new_individual_type(.5f); //FIXME there shouldn't be a .5 fixed value
        nextName[0] = rnd_char();
        create_individual(nextType,nextName,rnd_genome(2, genes));//Only the father will return from this call
    } 
    
    //****************************************************************
    //SIMULATION IS RUNNING
    //**************************************************************** 
    sops.sem_op = init_people;
	semop(semid, &sops, 1); //All individuals can start simultaneously now 

/*******IMPORTANT********/
//FIXME they don't start simultaneously, there's too much code between the fork and when the child does actually wait on the semaphore... the father process unlocks it way before every child is stuck on that semaphore. just notice how only a bunch of individuals (most of the times 3) get stuck on the semaphore before the father unlocks it! i think children should wait on the semafore even before the execve.

    printf("Children can go!\n");
    fflush(stdout);
    state = RUNNING; 
    alarm(birth_death); //Will send sigalarm every birth_death seconds
    sleep(10); //just a test to trigger the alarm

    //****************************************************************
    //CONCLUSION OF SIMULATION / PRINT STATISTICS
    //****************************************************************

    state = FINISHED;//this could be moved to the handler of the end of simulation

    while ((pid = wait(&status)) != -1) {
        printf("Got info of child with PID=%d, status=0x%04X\n", pid, status); //uncomment to see each exit status
    } //"kill" all zombies!
    if(errno == ECHILD) {
		printf("In PID=%6d, no more child processes\n", getpid());
        printf("The population was A:%d, B:%d\n", pop_a, pop_b);
		exit(EXIT_SUCCESS);
	}else {
		TEST_ERROR;
		exit(EXIT_FAILURE);
	}
        
	return 0;
}

//****************************************************************
//INDIVIDUALS CREATION FUNCTIONS
//****************************************************************

char new_individual_type(float a_type_probability){
    srand(getpid() + time(NULL) + pop_a + pop_b);//FIXME This works but it's weak and ugly, needs replacement
    return rand()%100 <= a_type_probability * 100.0 ? 'A' : 'B'; //random type
}

unsigned long rnd_genome(int x, unsigned long genes){
    srand(getpid() + time(NULL) + pop_a + pop_b);//FIXME This works but it's weak and ugly, needs replacement
    return rand()%genes+x; //Random unsigned long from x to genes+x
}

char rnd_char(){
    srand(getpid() + time(NULL) + pop_a + pop_b);//FIXME This works but it's weak and ugly, needs replacement
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
