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