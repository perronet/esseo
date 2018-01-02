#Wanna know how it works? http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
#gcc commands? http://www.network-theory.co.uk/docs/gccintro/gccintro_8.html
life_simulator : projectFiles
	gcc individual.c -o individual.out life_sim_lib.c & gcc life_simulator.c life_sim_lib.c life_sim_stats.c
projectFiles : life_simulator.c individual.c life_sim_lib.h life_sim_lib.c life_sim_stats.c
run: life_simulator
	sleep 1
	./a.out
test_session: life_simulator
	for number in 1 2 3 4 5 6 7 8 9 10; do \
	printf "\n\n\n\n\n\n" ; \
	printf "****************************************\n" ; \
	echo try $$number starting in; \
	printf "3...\n"  ; \
	sleep 1;\
	printf "2...\n"  ; \
	sleep 1;\
	printf "1...\n"  ; \
	sleep 1;\
	./a.out ; \
	done && \
	printf "################################################### \n SUCCESS :D \n###################################################\n"