OBJS = jms_coord.o jms_console.o pool.o sleep.o
OBJS2 = functions.o lists.o input.o tests.o arglist.o mystring.o
SOURCE = jms_coord.c jms_console.c functions.c lists.c input.c tests.c pool.c arglist.c mystring.c
HEADER = functions.h lists.h input.h arglist.h mystring.h
CC = gcc
CFLAGS= -c -Wall $(DEBUG)
LFLAGS= -Wall $(DEBUG)

all: jms_coord jms_console pool sleep

jms_coord: jms_coord.o functions.o lists.o mystring.o
	$(CC) $(LFLAGS) jms_coord.o functions.o lists.o mystring.o -o jms_coord

jms_console: jms_console.o functions.o input.o
	$(CC) $(LFLAGS) jms_console.o functions.o input.o -o jms_console

pool: pool.o functions.o lists.o arglist.o
	$(CC) $(LFLAGS) pool.o functions.o lists.o arglist.o -o pool

sleep: sleep.o
	$(CC) $(LFLAGS) sleep.o -o sleep

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

mystring.o: mystring.c mystring.h
	$(CC) $(CFLAGS) mystring.c

input.o: input.c input.h
	$(CC) $(CFLAGS) input.c	

arglist.o: arglist.c arglist.h
	$(CC) $(CFLAGS) arglist.c

sleep.o: sleep.c
	$(CC) $(CFLAGS) sleep.c

clean:
	rm -f $(OBJS) $(OBJS2) jms_console jms_coord pool sleep

count:
	wc $(SOURCE) $(HEADER)