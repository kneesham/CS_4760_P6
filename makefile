CC=gcc
SRC = $(wildcard *.c)               # list of source files
OBJS = $(patsubst %.c, %.o, $(SRC)) # list of object files
OSSOB = main.o bits.o
PRCSOB = userProcess.o

CFLAGS=-Wall -lm

all: objs oss user_proc

user_proc: userProcess.c
	$(CC) -o $@ $(PRCSOB)

oss:  main.c bits.c 
	$(CC) $(CFLAGS) -o $@ $(OSSOB)

#Create all the target object files.
objs : $(OBJS)

clean:
	rm oss user_proc  *.o  logfile

.PHONY: all clean
