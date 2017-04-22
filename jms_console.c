#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE 1024

#include "functions.h"
#include "input.h"

int main(int argc, char* argv[])
{
	char* jms_in = NULL;
	char* jms_out = NULL;
	char* operations_name = NULL;
	FILE* operations_file = NULL;
	int send_fd, receive_fd;
	int pid, coord_pid;
	char message[BUFSIZE];
	int index = 1;

	//Read Arguments
	while (index + 1 < argc)
	{
		if ( !strcmp(argv[index], "-w"))
		{
			jms_in = argv[index + 1];
			index++;
		}
		else if ( !strcmp(argv[index], "-r"))
		{
			jms_out = argv[index + 1];
			index++;
		}
		else if ( !strcmp(argv[index], "-o"))
		{
			operations_name = argv[index + 1];
			index++;
		}
		else
		{
			fprintf(stderr, "Error!Unknown parameter: %s\n", argv[index]);
			fprintf(stderr, "Usage: ./jms_console -w <jms_in> -r <jms_out> -o <operations_file>\n");
			return -1;
		}

		index++;
	}

	if (jms_in == NULL)
	{
		fprintf(stderr, "Error!jms_in was not given\n");
		fprintf(stderr, "Usage: ./jms_console -w <jms_in> -r <jms_out> -o <operations_file>\n");
		return -2;
	}
	if (jms_out == NULL)
	{
		fprintf(stderr, "Error!jms_out was not given\n");
		fprintf(stderr, "Usage: ./jms_console -w <jms_in> -r <jms_out> -o <operations_file>\n");
		return -2;
	}

	//Open pipes and "Handshake" with coord
	if ( (send_fd = open(jms_in, O_WRONLY)) < 0)
	{
		perror("Cannot open jms_in pipe");
		return -1;
	}

	pid = getpid();	
	if (PRINTS == 1)
		fprintf(stderr, "Console: My pid is %d.Sending it to coord\n", pid);
	sprintf(message, "%d", pid);
	write(send_fd, message, BUFSIZE);//Send coord my pid so he knows who I am

	if ( (receive_fd = open(jms_out, O_RDONLY)) < 0)
	{
		perror("Cannot open jms_out pipe");
		return -1;
	}

	read(receive_fd, message, BUFSIZE);//Receive coords pid
	coord_pid = atoi(message);
		if (PRINTS == 1)
	fprintf(stderr, "Console: Received coord's pid (%d)\n", coord_pid);

	//Send commands to coord after checking if they are valid (see file input.c)

	//first read from operations_file if there is one
	if (operations_name != NULL)
	{
		operations_file = fopen(operations_name, "r");
		fprintf(stderr, "---Sending commands from operations file '%s'.Please wait---\n\n", operations_name);
		send_input(operations_file, 10, 100, coord_pid, receive_fd, send_fd);
		fprintf(stderr, "---Done with operations file---\n\n");
	}

	fprintf(stderr, "---Reading commands from standart output.Use command 'exit' to terminate console---\n\n");
	send_input(stdin, 10, 100, coord_pid, receive_fd, send_fd);

	close(send_fd);
	close(receive_fd);

	if ( unlink(jms_in) < 0)
	{
		perror("Console: Cannot unlink");
	}
	if ( unlink(jms_out) < 0)
	{
		perror("Console: Cannot unlink");
	}


	return 0;
}