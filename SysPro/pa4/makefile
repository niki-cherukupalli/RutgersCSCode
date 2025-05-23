all: mcached

mcached: mcached.c mcached.h
	gcc -Wall -std=gnu11 -o mcached mcached.c -lpthread -lrt

clean: 
	rm -f mcached