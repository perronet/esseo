#include "lifeSimLib.h"

unsigned long string_to_ulong(char * c)
{
   	char *ptr;
  	return strtoul(c, &ptr, 10);
}

void ind_data_cpy(ind_data * dest, ind_data * src)
{
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
