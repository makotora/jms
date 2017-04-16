#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arglist.h"


int arglist_create(listofargs ** arglist)
{
	*arglist = malloc(sizeof(listofargs));

	if (*arglist == NULL) return 10;

	(*arglist)->first = NULL;
	(*arglist)->last = NULL;
	(*arglist)->counter = 0;

	return 0;
}



int arglist_add(listofargs * arglist, char* arg)
{
	argNode * new_arg = malloc(sizeof(argNode));
	new_arg->arg = malloc((strlen(arg) + 1)*sizeof(char));
	strcpy(new_arg->arg, arg);
	new_arg->next = NULL;

	if (arglist->first == NULL)
	{
		arglist->first = new_arg;
		arglist->last = new_arg;
	}
	else
	{
		arglist->last->next = new_arg;
		arglist->last = new_arg;	
	}

	arglist->counter++;

	return 0;
}



int arglist_free(listofargs ** arglist)
{
	if (*arglist == NULL)/*no list to free*/
	{
		return 1;
	}

	argNode * current;
	argNode * next;

	current = (*arglist)->first;

	while (current != NULL)
	{
		next = current->next;
		free(current->arg);
		free(current);
		current = next;
	}

	free(*arglist);
	*arglist = NULL;

	return 0;
}


char** arglist_to_array(listofargs* arglist)
{
	char** arg_array = malloc((arglist->counter+1) * sizeof(char*));//+1 because we want one spot for NULL of execv
	char* arg;
	int index = 0;

	argNode* current = arglist->first;

	while (current != NULL)
	{
		arg = current->arg;
		arg_array[index] = malloc((strlen(arg) + 1)*sizeof(char));
		strcpy(arg_array[index], arg);

		index++;
		current = current->next;
	}

	arg_array[index] = NULL;

	return arg_array;
}


void free_arg_array(char** arg_array)
{
	int index = 0;

	while (arg_array[index] != NULL)
	{
		free(arg_array[index]);
		index++;
	}
}