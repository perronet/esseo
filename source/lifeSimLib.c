#include "lifeSimLib.h"

unsigned long string_to_ulong(char * c){
   	char *ptr;
  	return strtoul(c, &ptr, 10);
}

unsigned long gcd(unsigned long a, unsigned long b){
    int temp;
    while (b != 0)
    {
        temp = a % b;
        a = b;
        b = temp;
    }
    return a;
}

void ind_data_cpy(ind_data * dest, ind_data * src){
	dest->type = src->type;
	strcpy(dest->name,src->name);
	dest->genome = src->genome;
	dest->pid = src->pid;
}

shared_data * get_shared_data(){
    shared_data * infoshared;
    int memid;
    memid = shmget(SHM_KEY, sizeof(shared_data), 0666 | IPC_CREAT);
    TEST_ERROR;
    infoshared = shmat(memid, NULL, 0); //attach pointer
    TEST_ERROR;
    return infoshared;
}

bool remove_from_agenda(ind_data * agenda, pid_t individual){
	for(int i = 0; i < MAX_AGENDA_LEN; i++)
	{
		if(IS_TYPE_A(agenda[i].type) && agenda[i].pid == individual)
		{
			agenda[i].type = 'X';//invalid type, indicates empty space
			return true;
		}
	}

	return false;//nothing found
}

void print_agenda(ind_data * agenda)
{
	LOG(LT_AGENDA_STATUS,"CURRENT AGENDA ########################\n");
	for(int i = 0; i < MAX_AGENDA_LEN; i++)
	{
		if(IS_TYPE_A(agenda[i].type))
		{
			printf("index %d with data: type: %c name: %s genome: %lu pid: %d \n", i,agenda[i].type, agenda[i].name, agenda[i].genome,agenda[i].pid);
		}
	}
	LOG(LT_AGENDA_STATUS,"########################\n");
}