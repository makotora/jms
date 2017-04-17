OBJS = jms_coord.o jms_console.o pool.o
OBJS2 = functions.o lists.o input.o tests.o arglist.o queue.o
SOURCE = jms_coord.c jms_console.c functions.c lists.c input.c tests.c pool.c arglist.c queue.c
HEADER = functions.h lists.h input.h arglist.h queue.h
CC = gcc
CFLAGS= -c -Wall $(DEBUG)
LFLAGS= -Wall $(DEBUG)

all: jms_coord jms_console pool

test: $(OBJS2)
	$(CC) $(LFLAGS) $(OBJS2) -o test

jms_coord: jms_coord.o functions.o lists.o
	$(CC) $(LFLAGS) jms_coord.o functions.o lists.o -o jms_coord

jms_console: jms_console.o functions.o input.o
	$(CC) $(LFLAGS) jms_console.o functions.o input.o -o jms_console

pool: pool.o functions.o lists.o arglist.o queue.o
	$(CC) $(LFLAGS) pool.o functions.o lists.o queue.o arglist.o -o pool

jms_coord.o: jms_coord.c
	$(CC) $(CFLAGS) jms_coord.c

jms_console.o: jms_console.c
	$(CC) $(CFLAGS) jms_console.c

pool.o: pool.c
	$(CC) $(CFLAGS) pool.c

functions.o: functions.c functions.h
	$(CC) $(CFLAGS) functions.c

lists.o: lists.c lists.h
	$(CC) $(CFLAGS) lists.c

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) queue.c

input.o: input.c input.h
	$(CC) $(CFLAGS) input.c	

arglist.o: arglist.c arglist.h
	$(CC) $(CFLAGS) arglist.c

tests.o: tests.c
	$(CC) $(CFLAGS) tests.c	

clean:
	rm -f $(OBJS) $(OBJS2) jms_console jms_coord test pool

count:
	wc $(SOURCE) $(HEADER)