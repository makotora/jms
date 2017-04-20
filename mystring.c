#include "mystring.h"

mystring* mystring_create(int capacity)
{
	mystring* newstring = malloc(sizeof(mystring));

	newstring->string = malloc(capacity*sizeof(char));
	newstring->size = 0;
	newstring->capacity = capacity;

	return newstring;
}


void mystring_free(mystring** string_dp)
{
	free((*string_dp)->string);
	free(*string_dp);
}


int mystring_add(mystring* string_p, char* str)
{
	int str_size = strlen(str);
	if (str_size + string_p->size < string_p->capacity)
	{//it fits!
		strcpy( (string_p->string + string_p->size), str);
		string_p->size += str_size;

		return 0;
	}
	else
	{
		fprintf(stderr, "MYSTRING ERROR: STRING FULL!CANNOT ADD\n");
		return -1;
	}
}


void mystring_clear(mystring* string_p)
{
	string_p->size = 0;
}