#include "lifeSimLib.h"

ind_data info;
int semid, msgid;

//Here is implemented the behaviour of "A" type individuals
void a_behaviour();
//Here is implemented the behaviour of "B" type individuals
void b_behaviour();


int main(int argc, char *argv[]){

    //****************************************************************
    //SETTING UP PERSONAL INFORMATION
    //****************************************************************
    semid = semget(SEMAPHORE_SET_KEY, 2, 0666);//FIXME replace key
    msgid = msgget(MSGQ_KEY, 0666);//FIXME replace key
	TEST_ERROR;
    
    bool wait_to_begin = false;//when the individual starts, it could have to wait before beginning its behaviour. This is passed via arguments
    info.pid = getpid();
    if(argc >= 5)
    {//Read data. Currently the positions of this values are hardcoded. We do not expect them to be scrambled
        info.type = *argv[1];
        strcpy(info.name, argv[2]);
        info.genome = string_to_ulong(argv[3]);
        wait_to_begin = *argv[4];
    }
    
    if(wait_to_begin)
    {
	    sops.sem_num = SEM_NUM_INIT;
		sops.sem_flg = 0; 
	    sops.sem_op = -1;
        printf("pid %d WAITING!\n", getpid());
        fflush(stdout);
        semop(semid, &sops, 1);//Decrement sem by one
		TEST_ERROR;
	    sops.sem_op = 0;
        semop(semid, &sops, 1);//Waits for semaphore to become 0 then everybody starts simultaneously
		TEST_ERROR;
        printf("pid %d Let's go!\n", getpid());
        fflush(stdout);
    }

    //printf("Hello! my PID is: %d, i'm type %c, my name is %s, my genome is %li\n", getpid(), info.type, info.name, info.genome);

    //****************************************************************
    //EXECUTE BEHAVIOUR
    //****************************************************************

    sops.sem_num = SEM_NUM_MUTEX;//Will always use semaphore mutex from now on (it's initialized to 1)
    if(info.type == 'A')
        a_behaviour();
    else if(IS_TYPE_B(info.type))
        b_behaviour();
    else
        CHECK_VALID_IND_TYPE(info.type)//this is a redundant check but I use it to log the error. In my defence, this should never happen.
    
    //TODO detach shared memory pointer and message queue, need to mark shared memory for deletion from lifeSimulator
    exit(EXIT_SUCCESS);
}

//****************************************************************
//BEHAVIOUR IMPLEMENTATION
//****************************************************************

void a_behaviour(){
    shared_data * infoshared = get_shared_data();
    msgbuf mex;
    sops.sem_op = -1;
    semop(semid, &sops, 1);//Accessing resource
    for(int i = 0; i < MAX_AGENDA_LEN; i++)
    {//Find a nice spot to place information
        //ind_data * slot = infoshared->agenda[i]; 
        //if(!IS_TYPE_A(slot->type))
        //{//This slot is free

        //}
    }
    sops.sem_op = 1;
    semop(semid, &sops, 1);//Releasing resource
    //Test message queue
    msgrcv(msgid, &mex, MSG_LEN, 0, 0);
    //FIXME can't print struct info properly! for some reason it prints fine in b_behaviour but prints random shit here! (we should use pointers to struct and not copy entire structs!!!)
    printf("Process A %d, i received:%s. here's your (wrong) info:%c, %lu, %d\n", getpid(), mex.mtext, mex.info.type, mex.info.genome, mex.info.pid);
    fflush(stdout);
}

void b_behaviour(){
    msgbuf mex;
    shared_data * infoshared = get_shared_data();
    sops.sem_op = -1;
    semop(semid, &sops, 1);//Accessing resource
    for(int i = 0; i < MAX_AGENDA_LEN; i++)
    {//Find a possible partner
    }
    sops.sem_op = 1;
    semop(semid, &sops, 1);//Releasing resource
    //Test message queue
    mex.mtype = 333;//TODO Should actually be set with the pid of the A process (get it from shared memory segment)
    ind_data_cpy(&(mex.info), &info);
    //Testing...
    printf("Info in message:%c, %lu, %d\n", mex.info.type, mex.info.genome, mex.info.pid); 
    fflush(stdout);
    strcpy(mex.mtext, "CIAO sono un processo B, vuoi accoppiarti?");
    msgsnd(msgid, &mex, MSG_LEN, 0);
}


//****************************************************************
//SIGNAL & MESSAGE HANDLING
//****************************************************************


//****************************************************************
//HELPER FUNCTIONS
//****************************************************************

