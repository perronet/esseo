#include "life_sim_stats.h"
#include <string.h>

//Sets up the stats
void init_stats(life_sim_stats * stats)
{
	stats->total_population_a = 0;
	stats->total_population_b = 0;
	stats->total_killed = 0;
	stats->total_couples = 0;
	stats->longest_name_individual.name[0] = '\0';
	stats->highest_genome_individual.genome = 0;
}

void register_individual_in_stats(life_sim_stats * stats, ind_data * individual)
{
	if(IS_TYPE_A(individual->type))
		stats->total_population_a ++;
	else
		stats->total_population_b ++;

	if(strlen(individual->name) > strlen(stats->longest_name_individual.name))
		ind_data_cpy(&stats->longest_name_individual,individual);

	if(individual->genome > stats->highest_genome_individual.genome)
		ind_data_cpy(&stats->highest_genome_individual,individual);
}

void print_stats(life_sim_stats * stats)
{
	LOG(LT_SHIPPING,"\n********** SIMULATION RESULTS **********\n");
	LOG(LT_SHIPPING,"*                                      *\n");
	LOG(LT_SHIPPING,"*    Total 'A' individuals  : %5u    *\n",stats->total_population_a);
	LOG(LT_SHIPPING,"*    Total 'B' individuals  : %5u    *\n",stats->total_population_b);
	LOG(LT_SHIPPING,"*    Total killed           : %5u    *\n",stats->total_killed);
	LOG(LT_SHIPPING,"*    Total couples          : %5u    *\n",stats->total_couples);
	LOG(LT_SHIPPING,"*                                      *\n");
	LOG(LT_SHIPPING,"*    Individual with longest name      *\n");
	LOG_INDIVIDUAL(LT_SHIPPING, stats->longest_name_individual);
	LOG(LT_SHIPPING,"*                                      *\n");
	LOG(LT_SHIPPING,"*    Individual with highest genome    *\n");
	LOG_INDIVIDUAL(LT_SHIPPING, stats->highest_genome_individual);
	LOG(LT_SHIPPING,"*                                      *\n");
	LOG(LT_SHIPPING,"****************************************\n\n");
}