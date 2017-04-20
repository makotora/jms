#ifndef MYSTRING_H
#define MYSTRING_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct mystring
{
	char* string;
	int size;
	int capacity;
};

typedef struct mystring mystring;

mystring* mystring_create(int capacity);
void mystring_free(mystring** string_dp);
int mystring_add(mystring* string_p, char* str);
void mystring_clear(mystring* string_p);

#endif