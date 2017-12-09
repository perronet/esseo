#Wanna know how it works? http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
#gcc commands? http://www.network-theory.co.uk/docs/gccintro/gccintro_8.html
lifeSimulator : projectFiles
	gcc individual.c -o individual.out lifeSimLib.c & gcc lifeSimulator.c lifeSimLib.c 

projectFiles : lifeSimulator.c individual.c lifeSimLib.h lifeSimLib.c

run: lifeSimulator
	./a.out
