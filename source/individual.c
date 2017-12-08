#include "lifeSimLib.h"

char rnd_char(){
    srand(getpid()); //getpid() as seed is better, time(NULL) could generate many child with the same seed
    return (rand()%26)+65; //random name 
}

unsigned long rnd_genome(int x, unsigned long genes){
    srand(getpid());
    return rand()%((x+genes)+1-x)+x; 
}

int main(int argc, char *argv[]){ 
    int i;
    int memid, msgq_id, num_bytes; //will use message queue
    long rcv_type;
	msgbuf my_msg;
    data info;
    shared_data * infoshared; 
    if(argc == 2){
        info.type = 'a';
    }else{
        info.type = 'b';
    }
    info.name[0] = rnd_char();
    info.genome = rnd_genome(50, 100);
    printf("Hello! my PID is: %d, i'm type %c, my name is %c, my genome is %li\n", getpid(), info.type, info.name[0], info.genome);

    memid = shmget(IPC_PRIVATE, sizeof(*infoshared), 0666);
	TEST_ERROR;
	infoshared = shmat(memid, NULL, 0); //attach pointer
	TEST_ERROR;
    //should test shared memory here
    exit(EXIT_SUCCESS);
	return 0;
}
