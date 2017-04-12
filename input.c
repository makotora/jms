#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "protocol.h"

int send_input(FILE* input, int max_args, int max_len, int coord_pid, int readfd, int writefd)
{
	char* line = malloc(max_len*sizeof(char));
	char* command = malloc(max_len*sizeof(char));
	char** words = malloc(max_args*sizeof(char*));
	int commands_count = 0;
	int i;

	while (1)
	{
		if (fgets(line,max_len, input) == NULL)
			break;

		/*ignore blank lines*/
		if (strlen(line) == 0)
			continue;

		commands_count++;
		strcpy(command, line);

		/*print the line you just read*/
		fprintf(stderr, "%s", line);

		for (i=0;i<max_args;i++)
			words[i] = NULL;

		/*split line to words*/
		words[0] = strtok(line," \t\n");
		int index = 0;

		do
		{
			index++;
			words[index] = strtok(NULL," \t\n");
		}
		while(words[index] != NULL && index < max_args);

		/*Debug.print words we read/split*/
		// for (i=0;i<index;i++)
		// 	fprintf(stderr, "%s\n", words[i]);


		/*CHECK IF COMMAND IS VALID BEFORE SENDING IT TO COORD*/
		if ( !strcmp(words[0], "submit"))
		{
			if (words[1] == NULL)
			{
				fprintf(stderr, "Invalid command! Job name and args required for submit command\n");
				continue;
			}
		}
		else if ( !strcmp(words[0], "status"))
		{
			if (words[1] == NULL)
			{
				fprintf(stderr, "Invalid command! JobID required for status command\n");
				continue;
			}
			else if (words[2] != NULL) //Too many arguments!Must be 2
			{
				fprintf(stderr, "Too many arguments! Only JobID is required for status command\n");
				continue;
			}
		}
		else if ( !strcmp(words[0], "suspend"))
		{
			if (words[1] == NULL)
			{
				fprintf(stderr, "Invalid command! JobID required for suspend command\n");
				continue;
			}
			else if (words[2] != NULL) //Too many arguments!Must be 2
			{
				fprintf(stderr, "Too many arguments! Only JobID is required for suspend command\n");
				continue;
			}
		}
		else if ( !strcmp(words[0], "resume"))
		{
			if (words[1] == NULL)
			{
				fprintf(stderr, "Invalid command! JobID required for resume command\n");
				continue;
			}
			else if (words[2] != NULL) //Too many arguments!Must be 2
			{
				fprintf(stderr, "Too many arguments! Only JobID is required for resume command\n");
				continue;
			}
		}
		else
		{
			if (!strcmp(words[0], "status-all") || !strcmp(words[0], "show-active") || !strcmp(words[0], "show-pools")
				|| !strcmp(words[0], "show-finished") || !strcmp(words[0], "shutdown"))
			{
				if (words[1] != NULL) //Too many arguments!Must be 1
				{
					fprintf(stderr, "Too many arguments!\n");
					continue;
				}
			}
			else
			{
				fprintf(stderr, "Unknown command: %s", command);		
				continue;	
			}
		}

		//If we didnt continue.Command was correct.Send it to coord
		send_message(command, coord_pid, readfd, writefd);

	}

	free(line);
	free(words);

	return commands_count;
}
