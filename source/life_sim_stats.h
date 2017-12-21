#include "lifeSimLib.h"

typedef struct {
	unsigned int total_population_a;
	unsigned int total_population_b;
	unsigned int total_killed;
	unsigned int total_couples;
	ind_data longest_name_individual;
	ind_data highest_genome_individual;
} life_sim_stats;

//Sets up the stats
void init_stats(life_sim_stats * stats);

void register_individual_in_stats(life_sim_stats * stats, ind_data * individual);

void print_stats(life_sim_stats * stats);