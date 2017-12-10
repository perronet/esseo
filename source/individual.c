#include "lifeSimLib.h"

ind_data info;
int semid;

//Here is implemented the behaviour of "A" type individuals
void a_behaviour();
//Here is implemented the behaviour of "B" type individuals
void b_behaviour();


int main(int argc, char *argv[]){

    //****************************************************************
    //SETTING UP PERSONAL INFORMATION
    //****************************************************************
    bool wait_to_begin = false;//when the individual starts, it could have to wait before beginning its behaviour. This is passed via arguments
    if(argc >= 5)
    {//Read data. Currently the positions of this values are hardcoded. We do not expect them to be scrambled
        info.type = *argv[1];
        strcpy(info.name, argv[2]);
        info.genome = string_to_ulong(argv[3]);
        wait_to_begin = *argv[4];
    }
    //printf("Hello! my PID is: %d, i'm type %c, my name is %s, my genome is %li\n", getpid(), info.type, info.name, info.genome);
    //Set semaphore operations
    semid = semget(123456789, 2, 0666); //FIXME I USED A HARDCODED KEY
	TEST_ERROR;
    sops.sem_num = 0;//check the 0-th semaphore
	sops.sem_flg = 0; 
    sops.sem_op = -1;
    if(wait_to_begin)
    {
        printf("pid %d WAITING!\n", getpid());
        fflush(stdout);
        semop(semid, &sops, 1);
        printf("pid %d Let's go!\n", getpid());
        fflush(stdout);
    }

    //****************************************************************
    //EXECUTE BEHAVIOUR
    //****************************************************************

    sops.sem_num = 1;//Will always use semaphore 1 from now on (it's initialized to 1)
    if(info.type == 'A')
        a_behaviour();
    else if(IS_TYPE_B(info.type))
        b_behaviour();
    else
        CHECK_VALID_IND_TYPE(info.type)//this is a redundant check but I use it to log the error. In my defence, this should never happen.
    
    exit(EXIT_SUCCESS);
}

//****************************************************************
//BEHAVIOUR IMPLEMENTATION
//****************************************************************

void a_behaviour(){
    shared_data * infoshared = get_shared_data();
    sops.sem_op = -1;
    semop(semid, &sops, 1);//Accessing resource
    printf("Let's publish shit!\n");
    for(int i = 0; i < MAX_AGENDA_LEN; i++)
    {//Find a nice spot to place information
        //ind_data * slot = infoshared->agenda[i]; 
        //if(!IS_TYPE_A(slot->type))
        //{//This slot is free

        //}
    }
    sops.sem_op = 1;
    semop(semid, &sops, 1);//Releasing resource
}

void b_behaviour(){
    shared_data * infoshared = get_shared_data();
    sops.sem_op = -1;
    semop(semid, &sops, 1);//Accessing resource
    printf("Let's find bitches!\n");
    for(int i = 0; i < MAX_AGENDA_LEN; i++)
    {//Find a possible partner
    }
    sops.sem_op = 1;
    semop(semid, &sops, 1);//Releasing resource
}


//****************************************************************
//SIGNAL & MESSAGE HANDLING
//****************************************************************


//****************************************************************
//HELPER FUNCTIONS
//****************************************************************

