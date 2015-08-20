# requires libsndfile to work
CC=g++

# -std=c++11 here for std::stoi std::stof
CFLAGS=-g -std=c++11

LDFLAGS=-L/usr/local/lib -lsndfile

all: sndchecker

sndchecker.o: sndchecker.cc
	g++ -c sndchecker.cc ${CFLAGS} -o sndchecker.o

sndchecker: sndchecker.o 
	g++ sndchecker.o ${CFLAGS} -o sndchecker ${LDFLAGS}

clean:
	rm *.o sndchecker

