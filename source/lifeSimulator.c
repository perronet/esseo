#include "lifeSimLib.h"

unsigned int birth_death; //global
shared_data * shared_info;

void handle_sigalarm(int signal) { 
    //handle birth_death events (kill a child, create a new child, print stats)
    alarm(birth_death); //schedule another alarm
    printf("ALARM!\n");
}

//Creates a new type, basing on the given probability of getting a type a. Probability is given from 0 to 1
char new_individual_type(float a_type_probability){
    srand(getpid() + time(NULL) + shared_info->pop_a + shared_info->pop_b);//This works but it's weak and ugly, needs replacement
    printf("%d\n", rand()%100);
    return rand()%100 <= a_type_probability * 100.0 ? 'A' : 'B'; //random type
}

//Generates a random genome, distributed from x to genes+x
unsigned long rnd_genome(int x, unsigned long genes){
    srand(getpid() + time(NULL) + shared_info->pop_a + shared_info->pop_b);//This works but it's weak and ugly, needs replacement
    return rand()%genes+x; //Random unsigned long from x to genes+x
}

//Returns a random uppercase character, assuming they are all contiguos in encoding
char rnd_char(){
    srand(getpid() + time(NULL) + shared_info->pop_a + shared_info->pop_b);//This works but it's weak and ugly, needs replacement
    return (rand()%26)+ 'A'; //random name 
}

//Creates a new individual by forking and executing the individual process
void create_individual(char type, char * name, unsigned long genome)
{
    CHECK_VALID_IND_TYPE(type)

    if(type == 'A')
        shared_info->pop_a++;
    else
        shared_info->pop_b++;

    switch(fork()){
        case -1://Error occured
            TEST_ERROR;
            exit(EXIT_FAILURE);
            break; 
        case 0://Child process
            ;//This is necessary to make the compiler happy, since we cannot have declarations next to labels. A label can only be part of a statement and a declaration is not a statement
            char genome_arg[50];
            sprintf(genome_arg,"%lu",genome);
            char * argv[] = {INDIVIDUAL_FILE_NAME,&type,name,genome_arg, NULL};//File name goes first and NULL is last to respect standard
            execve(INDIVIDUAL_FILE_NAME,argv,NULL);
            TEST_ERROR;
            exit(EXIT_FAILURE);
            break;
        default://Father process
            break;
        }
}

int main(){
    unsigned int init_people = 20, // initial population value
    				sim_time = 40; // total duration of simulation
    birth_death = 4;//tick interval of random killing and rebirth
    unsigned long genes = 100;//initial max value of genome

    int status, i, memid;
    pid_t pid;
    struct sembuf sops;
    struct sigaction sa;
    
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
    memid = shmget(IPC_PRIVATE, sizeof(*shared_info), 0666);
	TEST_ERROR;
	shared_info = shmat(memid, NULL, 0); //attach pointer
	TEST_ERROR;
    //Init
    shared_info->pop_a = 0;
    shared_info->pop_b = 0;
    for(i=0;i<init_people;i++){
    	char nextType = new_individual_type(.5f);
        char nextName [MAX_NAME_LEN];
        nextName[0] = rnd_char();
        create_individual(nextType,nextName,rnd_genome(2, genes));//Only the father will return from this call //TODO random name and genome
    } 
    //let all child process blocked on the semaphore start at the same time here with a semop, then:    
    alarm(birth_death);
    sleep(10); //just a test to trigger the alarm

    while (wait(&status) != -1) { } //"kill" all zombies!
    if(errno == ECHILD) {
		printf("In PID=%6d, no more child processes\n", getpid());
        printf("The population was A:%d, B:%d\n", shared_info->pop_a, shared_info->pop_b);
		exit(EXIT_SUCCESS);
	}else {
		TEST_ERROR;
		exit(EXIT_FAILURE);
	}
        
	return 0;
}
