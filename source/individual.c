#include "lifeSimLib.h"

ind_data info;

//Attaches to shared memory and gets the shared memory structs
shared_data * get_shared_data();
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
    printf("Hello! my PID is: %d, i'm type %c, my name is %s, my genome is %li\n", getpid(), info.type, info.name, info.genome);

    if(wait_to_begin)
    {
        printf("WAITING!\n");
        //#TODO "POPULATE" SEMAPHORE wait
    }

    //****************************************************************
    //EXECUTE BEHAVIOUR
    //****************************************************************

    if(IS_TYPE_A(info.type))
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

    //#TODO "MUTEX" SEMAPHORE WAIT
    printf("Let's publish shit!\n");
    for(int i = 0; i < MAX_AGENDA_LEN; i++)
    {//Find a nice spot to place information
        //ind_data * slot = infoshared->agenda[i]; 
        //if(!IS_TYPE_A(slot->type))
        //{//This slot is free

        //}
    }
    //#TODO "MUTEX" SEMAPHORE SIGNAL
}

void b_behaviour(){
    shared_data * infoshared = get_shared_data();

    //#TODO "MUTEX" SEMAPHORE WAIT
    printf("Let's find bitches!\n");
    for(int i = 0; i < MAX_AGENDA_LEN; i++)
    {//Find a possible partner
    }
    //#TODO "MUTEX" SEMAPHORE SIGNAL
}


//****************************************************************
//SIGNAL & MESSAGE HANDLING
//****************************************************************


//****************************************************************
//HELPER FUNCTIONS
//****************************************************************

shared_data * get_shared_data(){
    shared_data * infoshared;
    int memid;
    memid = shmget(IPC_PRIVATE, sizeof(*infoshared), 0666);
    TEST_ERROR;
    infoshared = shmat(memid, NULL, 0); //attach pointer
    TEST_ERROR;
    return infoshared;
}
