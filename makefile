CC=gcc
CFLAGS=-Wall -O2
DBFLAGS=-g -Wall -DDEBUG


all: build

rc.o: rc.c rc.h
	$(CC) $(DBFLAGS) -c rc.c -o build/rc.o

ReleaseRc.o: rc.c rc.h
	$(CC) $(CFLAGS) -c rc.c -o build/rc.o

debug: main.c rc.o
	$(CC) $(DBFLAGS) $< -o build/rc_test

clean:
	rm build/rc.o
	rm build/rc_test

build: rc.o
	$(CC) $(CFLAGS) $< -o build/rc_test

