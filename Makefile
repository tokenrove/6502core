# Evil makefu

default: coreddt

coreddt:
	gcc -Wall -pedantic -ansi -g -o coreddt coreddt.c 6502core.c

clean:
	rm -f `find -name \*.o -name \*~`
