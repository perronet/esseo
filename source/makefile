#Wanna know how it works? http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
#gcc commands? http://www.network-theory.co.uk/docs/gccintro/gccintro_8.html
lifeSimulator : projectFiles
	gcc individual.c -o individual.out & gcc lifeSimulator.c

projectFiles : lifeSimulator.c individual.c lifeSimLib.h

run: lifeSimulator
	./a.out
