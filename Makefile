OBJS = jms_coord.o jms_console.o pool.o
OBJS2 = protocol.o lists.o input.o tests.o
SOURCE = jms_coord.c jms_console.c protocol.c lists.c input.c tests.c pool.c
HEADER = protocol.h lists.h input.h
CC = gcc
CFLAGS= -c -Wall $(DEBUG)
LFLAGS= -Wall $(DEBUG)

all: jms_coord jms_console pool

test: $(OBJS2)
	$(CC) $(LFLAGS) $(OBJS2) -o test

jms_coord: jms_coord.o protocol.o lists.o
	$(CC) $(LFLAGS) jms_coord.o protocol.o lists.o -o jms_coord

jms_console: jms_console.o protocol.o input.o
	$(CC) $(LFLAGS) jms_console.o protocol.o input.o -o jms_console

pool: pool.o protocol.o lists.o
	$(CC) $(LFLAGS) pool.o protocol.o lists.o -o pool

jms_coord.o: jms_coord.c
	$(CC) $(CFLAGS) jms_coord.c

jms_console.o: jms_console.c
	$(CC) $(CFLAGS) jms_console.c

pool.o: pool.c
	$(CC) $(CFLAGS) pool.c

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) protocol.c

lists.o: lists.c lists.h
	$(CC) $(CFLAGS) lists.c

input.o: input.c input.h
	$(CC) $(CFLAGS) input.c	

tests.o: tests.c
	$(CC) $(CFLAGS) tests.c	

clean:
	rm -f $(OBJS) $(OBJS2) jms_console jms_coord test pool