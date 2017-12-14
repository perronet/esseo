#include "lifeSimLib.h"

#define MUTEX_P sops.sem_num=SEM_NUM_MUTEX;\
                sops.sem_op = -1; \
                semop(semid, &sops, 1); TEST_ERROR /*ACCESSING*/sigprocmask(SIG_BLOCK, &my_mask, NULL);TEST_ERROR//Block SIGUSR1 signals 
    
#define MUTEX_V sops.sem_num=SEM_NUM_MUTEX;\
        sops.sem_op = 1; \
        semop(semid, &sops, 1); TEST_ERROR /*RELEASING*/sigprocmask(SIG_UNBLOCK, &my_mask, NULL);TEST_ERROR//Unblock SIGUSR1 signals 


ind_data info;
int semid, msgid;

//Here is implemented the behaviour of "A" type individuals
void a_behaviour();
//Here is implemented the behaviour of "B" type individuals
void b_behaviour();
//Send a message with the given type, message and individual data
void send_message(pid_t to, char msg, ind_data * content);
//Signal handler
void handle_sigusr(int signal);
//-1 on semaphore + block signals
void access_resource();
//+1 on semaphore + unblock signals
void release_resource();

int main(int argc, char *argv[]){

    //***Init of signal handlers and mask
    sa.sa_handler = &handle_sigusr; 
	sa.sa_flags = 0; 
	sigemptyset(&my_mask); 
    sa.sa_mask = my_mask; //do not mask any signal in handler
    sigaddset(&my_mask, SIGUSR1);
    sigaction(SIGUSR1, &sa, NULL);

    semid = semget(SEMAPHORE_SET_KEY, 2, 0666);//FIXME replace key
    msgid = msgget(MSGQ_KEY, 0666);//FIXME replace key
	TEST_ERROR;
    //****************************************************************
    //SETTING UP PERSONAL INFORMATION
    //****************************************************************    
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
        //printf("pid %d Let's go!\n", getpid());
        //fflush(stdout);
    }

    printf("Hello! my PID is: %d, i'm type %c, my name is %s, my genome is %li\n", getpid(), info.type, info.name, info.genome);

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
    
    //****************************************************************
    //PUBLISH INFO TO AGENDA
    //****************************************************************

    MUTEX_P

    bool found = false;
    for(int i = 0; i < MAX_AGENDA_LEN && !found; i++)
    {//Find a nice spot to place information
        ind_data * slot = &(infoshared->agenda[i]); 
        if(!IS_TYPE_A(slot->type))
        {//This slot is free
            printf("publishing data at index %d!\n", i);
            ind_data_cpy(slot, &info);
            found = true;
        }
    }
    
    MUTEX_V

    //****************************************************************
    //WAIT FOR PROPOSALS
    //****************************************************************
    msgbuf msg;
    forever
    {
        msgrcv(msgid, &msg, MSGBUF_LEN, getpid(), 0);

        //****************************************************************
        //THIS INDIVIDUAL HAS BEEN CONTACTED. LET'S LOOK
        //****************************************************************
        MUTEX_P
        
        printf("Process A %d was contacted by B %d\n",getpid(), msg.info.pid);

        //printf("ind type A %d, **** RECEIVED %c, %lu, %d\n", getpid(), msg.mtext, msg.info.type, msg.info.genome, msg.info.pid);
        if(msg.info.genome % info.genome == 0 ||true|| rand()%2)//TODO replace rand with actual heuristic
        {

            printf("Process A %d accepted B %d\n",getpid(), msg.info.pid);
            pid_t partner_pid = msg.info.pid;//Let's save partner's pid
            remove_from_agenda(infoshared->agenda, getpid());//Let's remove data from agenda, this individual will not be contacted anymore
            
            MUTEX_V

            msgbuf msg_to_refuse;
            while(errno != ENOMSG)
            {
                if(msgrcv(msgid, &msg_to_refuse, MSGBUF_LEN, getpid(), IPC_NOWAIT)!= -1)
                {//Let's turn down any other pending request
                    send_message(msg_to_refuse.info.pid, 'N', &info);
                }
            }

            printf("Process A sending back messages, has pid %d\n", getpid());
            send_message(partner_pid, 'Y',&info);//Communicating to process B acceptance
            send_message(getppid(), 'Y',&msg.info);//Communicating to parent the pid of the partner
            printf("Process SENT back messages, has pid %d\n", getpid());


            exit(EXIT_SUCCESS);//TODO MAYBE this should be removed, manager should take care of killing
        }

        MUTEX_V

        msgsnd(msgid, &msg, MSGBUF_LEN, 0);
    }
}

void b_behaviour(){
    shared_data * infoshared = get_shared_data();
    for(int i = 0; i < MAX_AGENDA_LEN; i++)
    {//Find a possible partner
        MUTEX_P

        if(IS_TYPE_A(infoshared->agenda[i].type))
        {
            if(true)//TODO ADD HEURISTIC OF REQUEST SENDING DECISION
            {
                printf("Process B %d contacting %d\n",getpid(), infoshared->agenda[i].pid);
                fflush(stdout);
                
                send_message(infoshared->agenda[i].pid,'Y', &info);

                MUTEX_V

                msgbuf msg;
                msgrcv(msgid, &msg, MSGBUF_LEN, getpid(), 0);//wait for response
                printf("Process B %d RECEIVED message! %c\n",getpid(), msg.mtext);

                MUTEX_P

                if(msg.mtext == 'Y')
                {//We got lucky
                    printf("Process B %d got lucky with %d\n",getpid(), msg.info.pid);
                    fflush(stdout);

                    send_message(getppid(), 'Y',&msg.info);//Communicating to parent the pid of the partner

                    MUTEX_V

                    exit(EXIT_SUCCESS);//TODO MAYBE this should be removed, manager should take care of killing
                }
                else
                    printf("Process B %d got REFUSED from %d\n",getpid(), msg.info.pid);

                MUTEX_V
            }
            else
            {
            	MUTEX_V
            }
        }
        else
        {
            MUTEX_V
        }

        if(i >= MAX_AGENDA_LEN-1)
            i = -1;//Finding the perfect partner is an hard task. Let's start again
    }


    //Test message queue
    //printf("SENDING***:%c, %lu, %d\n", msg.info.type, msg.info.genome, msg.info.pid); 
}


//****************************************************************
//SIGNAL & MESSAGE HANDLING
//****************************************************************

void send_message(pid_t to, char msg_text, ind_data * content)
{
    msgbuf msg;
    msg.mtype = to;
    msg.mtext = msg_text;
    ind_data_cpy(&(msg.info), content);
    msgsnd(msgid, &msg, MSGBUF_LEN, 0);
}

void handle_sigusr(int signal){ 
    //If here this individual is marked for death, must handle this kind of situation
    printf("pid %d marked for death!\n", getpid());
}

//****************************************************************
//HELPER FUNCTIONS
//****************************************************************

/*
//These two functions will always use the SEM_NUM_MUTEX semaphore
void access_resource(){
    sops.sem_op = -1;
    semop(semid, &sops, 1);//Accessing resource
    TEST_ERROR;
    sigprocmask(SIG_BLOCK, &my_mask, NULL);//Block SIGUSR1 signals 
    TEST_ERROR;
}

void release_resource(){
    sops.sem_op = 1;
    semop(semid, &sops, 1);//Releasing resource
    TEST_ERROR;
    sigprocmask(SIG_UNBLOCK, &my_mask, NULL);//Unblock SIGUSR1 signals
}*/
