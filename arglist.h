#ifndef ARGLIST_H
#define ARGLIST_H

typedef struct argNode argNode;
struct argNode
{
	char* arg;
	argNode * next;
};

typedef struct listofargs listofargs;
struct listofargs
{
	argNode * first;
	argNode * last;
	int counter;
};

int arglist_create(listofargs ** arglist);
int arglist_add(listofargs * arglist, char* arg);
int arglist_free(listofargs ** arglist);
char** arglist_to_array(listofargs* arglist);
void free_arg_array(char** arg_array);

#endif