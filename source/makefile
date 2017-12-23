#Wanna know how it works? http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
#gcc commands? http://www.network-theory.co.uk/docs/gccintro/gccintro_8.html
lifeSimulator : projectFiles
	gcc individual.c -o individual.out lifeSimLib.c & gcc lifeSimulator.c lifeSimLib.c life_sim_stats.c
projectFiles : lifeSimulator.c individual.c lifeSimLib.h lifeSimLib.c life_sim_stats.c
run: lifeSimulator
	sleep 1
	./a.out
test_session: lifeSimulator
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