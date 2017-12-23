#include "lifeSimLib.h"

#define TYPE_A_ADAPTMENT_STEPS 5.f //Each time an A individual finds out it cannot reproduce, it will decrease his acceptance threshold by a percentage towards 0 (0 = accepts anyone) following this steps
#define TYPE_B_ADAPTMENT_STEPS 5.f //Each time a B individual finds out it cannot reproduce, it will decrease his acceptance threshold by a percentage towards 0 (0 = contacts anyone) following this steps

#define MUTEX_P errno = 0; sops.sem_num=SEM_NUM_MUTEX;\
				sops.sem_op = -1; \
                semop(semid, &sops, 1); TEST_ERROR /*ACCESSING*/sigprocmask(SIG_BLOCK, &my_mask, NULL);TEST_ERROR//Block SIGUSR1 signals 
    
#define MUTEX_V errno = 0; sops.sem_num=SEM_NUM_MUTEX;\
				sops.sem_op = 1; \
        semop(semid, &sops, 1); TEST_ERROR /*RELEASING*/sigprocmask(SIG_UNBLOCK, &my_mask, NULL);TEST_ERROR//Unblock SIGUSR1 signals 


ind_data info;
int semid, msgid;
int acceptance_threshold; // Accepts a partner if gcd / genome >= acceptance_threshold / ADAPTMENT_STEPS
int refused_individuals_count; //Keep track of the number of refused individuals since last adaptment
shared_data * infoshared;

//Here is implemented the behaviour of "A" type individuals
void a_behaviour();
//Here is implemented the behaviour of "B" type individuals
void b_behaviour();
//Lowers the acceptance threshold and reset the refused counter if necessary
void check_and_adapt();
//Returns true if the partner is a good match for this individual, basing on the acceptance_threshold
bool evaluate_possible_partner(unsigned long genome);
//Send a message with the given type, message and individual data.
void send_message(pid_t to, char msg, ind_data * content);
//Signal handler
void handle_sigusr(int signal);
/*
//-1 on semaphore + block signals
void access_resource();
//+1 on semaphore + unblock signals
void release_resource();*/

int main(int argc, char *argv[]){

    LOG(LT_INDIVIDUALS_ACTIONS,"Hello! STILL TO DO STUFF: my PID is: %d\n", getpid());

    //***Init of signal handlers and mask
    sa.sa_handler = &handle_sigusr; 
	sa.sa_flags = 0; 
	sigemptyset(&my_mask); 
    sa.sa_mask = my_mask; //do not mask any signal in handler
    sigaddset(&my_mask, SIGUSR1);
    sigaction(SIGUSR1, &sa, NULL);
    TEST_ERROR;

    semid = semget(SEMAPHORE_SET_KEY, 2, 0666);
    TEST_ERROR;
    msgid = msgget(MSGQ_KEY, 0666);
	TEST_ERROR;
    //****************************************************************
    //SETTING UP PERSONAL INFORMATION
    //****************************************************************
    infoshared = get_shared_data();    
    bool wait_to_begin = false;//when the individual starts, it could have to wait before beginning its behaviour. This is passed via arguments
    refused_individuals_count = 0;
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
        LOG(LT_INDIVIDUALS_ACTIONS,"pid %d WAITING!\n", getpid()); 
        fflush(stdout);
        semop(semid, &sops, 1);//Decrement sem by one
		TEST_ERROR;
	    sops.sem_op = 0;
        semop(semid, &sops, 1);//Waits for semaphore to become 0 then everybody starts simultaneously
		TEST_ERROR;
    }

    LOG(LT_INDIVIDUALS_ACTIONS,"Hello! my PID is: %d, i'm type %c, my name is %s, my genome is %li\n", getpid(), info.type, info.name, info.genome);

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
}

//****************************************************************
//BEHAVIOUR IMPLEMENTATION
//****************************************************************

void a_behaviour(){
    acceptance_threshold = TYPE_A_ADAPTMENT_STEPS;
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
            LOG(LT_INDIVIDUALS_ACTIONS,"publishing data at index %d!\n", i);
            ind_data_cpy(slot, &info);
            found = true;
        }
    }
    
    infoshared->current_pop_a ++;
    MUTEX_V

    //****************************************************************
    //WAIT FOR PROPOSALS
    //****************************************************************
    msgbuf msg;
    forever
    {
    	TEST_ERROR
        msgrcv(msgid, &msg, MSGBUF_LEN, getpid(), 0);
        TEST_ERROR
        //****************************************************************
        //THIS INDIVIDUAL HAS BEEN CONTACTED. LET'S LOOK
        //****************************************************************
        MUTEX_P
        
        LOG(LT_INDIVIDUALS_ACTIONS,"Process A %d was contacted by B %d\n",getpid(), msg.info.pid);

#if CM_SAY_ALWAYS_YES
        if(true)
#else
        if(evaluate_possible_partner(msg.info.genome))
#endif
        {
#if CM_SLOW_MO
            sleep(SLOW_MO_SLEEP_TIME);
#endif
            LOG(LT_INDIVIDUALS_ACTIONS,"Process A %d accepted B %d\n",getpid(), msg.info.pid);
            pid_t partner_pid = msg.info.pid;//Let's save partner's pid
            remove_from_agenda(infoshared->agenda, getpid());//Let's remove data from agenda, this individual will not be contacted anymore
            remove_pid(infoshared->alive_individuals, getpid());//Let's remove pid to prevent broken love
            remove_pid(infoshared->alive_individuals, partner_pid);//Let's remove partner pid to prevent broken love
            
            infoshared->current_pop_a --;//We are going to die soon ='(


            msgbuf msg_to_refuse;
            while(errno != ENOMSG)
            {
                if(msgrcv(msgid, &msg_to_refuse, MSGBUF_LEN, getpid(), IPC_NOWAIT)!= -1)
                {//Let's turn down any other pending request
                    send_message(msg_to_refuse.info.pid, 'N', &info);
                }
            }

            LOG(LT_INDIVIDUALS_ACTIONS,"From %d: Process A sending back messages\n", getpid());
            
            MUTEX_V

#if CM_SLOW_MO
            sleep(SLOW_MO_SLEEP_TIME);
#endif
            send_message(partner_pid, 'Y',&info);//Communicating to process B acceptance
            send_message(getppid(), 'Y',&msg.info);//Communicating to parent the pid and data of the partner
            LOG(LT_INDIVIDUALS_ACTIONS,"From %d: Process A SENT back messages\n", getpid());

            exit(EXIT_SUCCESS);
        }
        else
        {
            send_message(msg.info.pid, 'N',&info);//Communicating to process B refusal
            refused_individuals_count ++;
            check_and_adapt();
        }

        MUTEX_V
    }
}

void b_behaviour(){
    acceptance_threshold = TYPE_B_ADAPTMENT_STEPS;

    MUTEX_P
    infoshared->current_pop_b ++;
    MUTEX_V

    for(int i = 0; i < MAX_AGENDA_LEN; i++) 
    {//Find a possible partner
        MUTEX_P

        if(IS_TYPE_A(infoshared->agenda[i].type))
        {
#if CM_SAY_ALWAYS_YES
            if(true)
#else
            if(evaluate_possible_partner(infoshared->agenda[i].genome))
#endif
            {
#if CM_SLOW_MO
                sleep(SLOW_MO_SLEEP_TIME);
#endif
                LOG(LT_INDIVIDUALS_ACTIONS,"Process B %d contacting %d\n",getpid(), infoshared->agenda[i].pid);
                
                send_message(infoshared->agenda[i].pid,'Y', &info);

                MUTEX_V

                msgbuf msg;
                msgrcv(msgid, &msg, MSGBUF_LEN, getpid(), 0);//wait for response
#if CM_SLOW_MO
                sleep(SLOW_MO_SLEEP_TIME);
#endif
                LOG(LT_INDIVIDUALS_ACTIONS,"Process B %d RECEIVED message! %c\n",getpid(), msg.mtext);

                MUTEX_P

                if(msg.mtext == 'Y') //TODO mask SIGUSR1 signals here 
                {//We got lucky
#if CM_SLOW_MO
                    sleep(SLOW_MO_SLEEP_TIME);
#endif

                    LOG(LT_INDIVIDUALS_ACTIONS,"Process B %d got lucky with %d\n",getpid(), msg.info.pid);

                    send_message(getpid(), 'Y',&msg.info);//Communicating to parent the pid and data of the partner
                    									  //Using mtype getpid() instead of getppid() so the father can associate this process with its partner
                    								      //Only the parent will read this message, this process won't receive messages for now on
                    
                    infoshared->current_pop_b --;
                    MUTEX_V
                    exit(EXIT_SUCCESS);
                }
                else
                {
                    LOG(LT_INDIVIDUALS_ACTIONS,"Process B %d got REFUSED from %d\n",getpid(), msg.info.pid);
                }

                MUTEX_V
            }
            else
            {//we don't like this partner
                MUTEX_V
                refused_individuals_count ++;
                check_and_adapt();
            }
        }
        else
        {
            MUTEX_V 
        }

        if(i >= MAX_AGENDA_LEN-1)
            i = -1;//Finding the perfect partner is a hard task. Let's start again
    }


    //Test message queue
    //LOG(LT_INDIVIDUALS_ACTIONS,"SENDING***:%c, %lu, %d\n", msg.info.type, msg.info.genome, msg.info.pid); 
}

bool evaluate_possible_partner(unsigned long genome)
{
    int gcdiv = gcd(genome, info.genome);
    float current_threshold = acceptance_threshold / (IS_TYPE_A(info.type) ? TYPE_A_ADAPTMENT_STEPS : TYPE_B_ADAPTMENT_STEPS);
    bool result = genome % info.genome == 0 /*<-- should be useless but let's put it to avoid floating point errors */
                    || gcdiv >= info.genome / 2.f * current_threshold;

    LOG(LT_INDIVIDUALS_ADAPTATION,"******Type %c Evaluating. genome: %lu parner genome: %lu, threshold: %f gdc: %d and result is %d\n",info.type, info.genome,genome,current_threshold,gcdiv,result);
    return result;
}

void check_and_adapt()
{
    LOG(LT_INDIVIDUALS_ADAPTATION, "Process type %c %d is adapting to %d!\n", info.type, getpid(),acceptance_threshold-1);
    if(refused_individuals_count > (IS_TYPE_A(info.type) ? infoshared->current_pop_b : infoshared->current_pop_a))
    {//we refused more individuals than there are now in the population since the last adaptment. Time to adapt again!
        if(acceptance_threshold > 0)
        {
            acceptance_threshold --;
            refused_individuals_count = 0;
        }
        else
        {
            LOG(LT_GENERIC_ERROR, "Process type %c %d tried to lower the acceptance_threshold to less than 0, this should never happen!\n", info.type, getpid());
        }
    }
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

    while(msgsnd(msgid, &msg, MSGBUF_LEN, 0) == -1)
    {
        //TODO give cpu to other processes
    }
}

void handle_sigusr(int signal){ 
    //If here this individual is marked for death, must handle this kind of situation
    LOG(LT_INDIVIDUALS_ACTIONS,"pid %d marked for DEATH!\n", getpid());
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
