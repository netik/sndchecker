# requires libsndfile to work
CC=g++

LDFLAGS=-L/usr/local/lib -lsndfile

all: sndchecker

sndchecker.o: sndchecker.cc

sndchecker: sndchecker.o 
	g++ sndchecker.o -o sndchecker ${LDFLAGS}


clean:
	rm *.o sndchecker


