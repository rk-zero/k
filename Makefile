CFLAGS=-lm -fopenmp
prog: memuni
	gcc ${CFLAGS} memuni.c -o memuni

