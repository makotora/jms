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
#define PERMS 0666

#include "protocol.h"
#include "lists.h"

int console_message = 0;

int main(int argc, char* argv[])
{
	char * path = NULL;
	int pool_max = -1;
	char * jms_in = NULL;
	char * jms_out = NULL;
	int receive_fd, send_fd;
	int status;
	int pid, console_pid;
	char message[1024];
	char pool_pipe_str[100]; 
	int pool_counter = 0;
	int job_counter = 0;

	//Read Arguments
	//..

	path = "temp";
	pool_max = 3;
	jms_in = "jmsin";
	jms_out = "jmsout";


	//Set a signal handler for signal 10 (user defined)
	void receive(int signum);
	signal(SIGUSR1, &receive);//this just send global var console_message to 1 (so we know we have to read the pipe)

	//Create pipes for communication with console
	if ( (mkfifo(jms_in, PERMS) < 0) && (errno != EEXIST) )
	{
		perror("Cannot create fifo");
	}

	if ( (mkfifo(jms_out, PERMS) < 0) && (errno != EEXIST) )
	{
		perror("Cannot create fifo");
	}

	fprintf(stderr, "Coord: Created pipes.About to try opening them\n");
	
	//Open pipes (block until console opens them as well)
	if ( (receive_fd = open(jms_in, O_RDONLY)) < 0)
	{
		perror("Cannot open jms_in pipe");
	}

	if ( (send_fd = open(jms_out, O_WRONLY)) < 0)
	{
		perror("Cannot open jms_out pipe");
	}

	fprintf(stderr, "Coord: Opened pipes!\n");

	pid = getpid();
	
	//"Handshake" with console
	read(receive_fd, message, BUFSIZE);
	console_pid = atoi(message);
	fprintf(stderr, "Coord: Received console's pid (%d)\n", console_pid);

	fprintf(stderr, "Coord: My pid is %d.Sending it to console\n", pid);
	sprintf(message, "%d", pid);
	write(send_fd, message, BUFSIZE);//Send my pid to console so he can signal me for messages

	while (1)
	{
		if (console_message == 1)
		{
			read(receive_fd, message, BUFSIZE);
			fprintf(stderr, "Coord: Console sent me: %s\n", message);
			strcpy(message, "OK");
			write(send_fd, message, BUFSIZE);
			console_message = 0;
		}
	}

	close(receive_fd);
	close(send_fd);

	return 0;
}


void receive(int signum)
{console_message = 1;}