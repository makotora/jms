#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE 1024
#define PRINTS 1

int write_and_read(char* message, char* reply, int readfd, int writefd);
int intToString(int integer,char **string_ptr);
void get_date_time_str(char* time, char* date);

#endif