#include "lifeSimLib.h"

unsigned long string_to_ulong(char * c){
   	char *ptr;
  	return strtoul(c, &ptr, 10);
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
    memid = shmget(IPC_PRIVATE, sizeof(*infoshared), 0666 | IPC_CREAT);
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