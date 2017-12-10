#include "lifeSimLib.h"

ind_data info;
int semid, msgid;

//Here is implemented the behaviour of "A" type individuals
void a_behaviour();
//Here is implemented the behaviour of "B" type individuals
void b_behaviour();
//Send a message with the given type, message and individual data
void send_message(pid_t to, char msg, ind_data * content);

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

    sops.sem_op = -1;
    semop(semid, &sops, 1);//Accessing resource

    ind_data * current_slot = NULL;
    for(int i = 0; i < MAX_AGENDA_LEN && !current_slot; i++)
    {//Find a nice spot to place information
        ind_data * slot = &(infoshared->agenda[i]); 
        if(!IS_TYPE_A(slot->type))
        {//This slot is free
            ind_data_cpy(slot, &info);
            current_slot = slot;
        }
    }
    sops.sem_op = 1;
    semop(semid, &sops, 1);//Releasing resource

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
        sops.sem_op = -1;
        semop(semid, &sops, 1);//Accessing resource
    	TEST_ERROR;
        
        printf("Process A %d was contacted by B %d\n",getpid(), msg.info.pid);

        //printf("ind type A %d, **** RECEIVED %c, %lu, %d\n", getpid(), msg.mtext, msg.info.type, msg.info.genome, msg.info.pid);
        if(msg.info.genome % info.genome == 0 ||true|| rand()%2)//TODO replace rand with actual heuristic
        {
            printf("Process A %d accepted B %d\n",getpid(), msg.info.pid);
            pid_t partner_pid = msg.info.pid;//Let's save partner's pid
            remove_from_agenda(infoshared->agenda, getpid());//Let's remove data from agenda, this individual will not be contacted anymore
            
            msgbuf msg_to_refuse;
            while(msgrcv(msgid, &msg_to_refuse, MSGBUF_LEN, getpid(), IPC_NOWAIT)!= -1);{//Let's turn down any other pending request
                send_message(msg_to_refuse.info.pid, 'N', &info);
            }

            send_message(partner_pid, 'Y',&info);//Communicating to process B acceptance
            send_message(getppid(), 'Y',&msg.info);//Communicating to parent the pid of the partner

            sops.sem_op = 1;
            semop(semid, &sops, 1);//Releasing resource

            exit(EXIT_SUCCESS);//TODO MAYBE this should be removed, manager should take care of killing
        }

        sops.sem_op = 1;
        semop(semid, &sops, 1);//Releasing resource

        msgsnd(msgid, &msg, MSGBUF_LEN, 0);
    }
}

void b_behaviour(){
    shared_data * infoshared = get_shared_data();
    for(int i = 0; i < MAX_AGENDA_LEN; i++)
    {//Find a possible partner
        sops.sem_op = -1;
        semop(semid, &sops, 1);//Accessing resource
        if(IS_TYPE_A(infoshared->agenda[i].type))
        {
            if(true)//TODO ADD HEURISTIC OF REQUEST SENDING DECISION
            {
                printf("Process B %d contacting %d\n",getpid(), infoshared->agenda[i].pid);
                fflush(stdout);
                
                send_message(infoshared->agenda[i].pid,'Y', &info);

                sops.sem_op = 1;
                semop(semid, &sops, 1);//Releasing resource

                msgbuf msg;
                msgrcv(msgid, &msg, MSGBUF_LEN, getpid(), 0);//wait for response

                sops.sem_op = -1;
                semop(semid, &sops, 1);//Accessing resource

                if(msg.mtext == 'Y')
                {//We got lucky
                    printf("Process B %d got lucky with %d\n",getpid(), msg.info.pid);
                    fflush(stdout);

                    send_message(getppid(), 'Y',&msg.info);//Communicating to parent the pid of the partner

                    sops.sem_op = 1;
                    semop(semid, &sops, 1);//Releasing resource

                    exit(EXIT_SUCCESS);//TODO MAYBE this should be removed, manager should take care of killing
                }
            }
            sops.sem_op = 1;
            semop(semid, &sops, 1);//Releasing resource
        }

        if(i >= MAX_AGENDA_LEN-1)
            i = 0;//Finding the perfect partner is an hard task. Let's start again
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
    ind_data_cpy(&(msg.info), content);
    msgsnd(msgid, &msg, MSGBUF_LEN, 0);
}

//****************************************************************
//HELPER FUNCTIONS
//****************************************************************

