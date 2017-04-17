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
	char * jms_in = NULL;
	char * jms_out = NULL;
	int send_fd, receive_fd;
	int pid, coord_pid;
	char message[BUFSIZE];

	//Read Arguments
	//..

	jms_in = "jmsin";//We write here (input for coord)
	jms_out = "jmsout";//We read from here (output for coord)

	//Open pipes and "Handshake" with coord
	if ( (send_fd = open(jms_in, O_WRONLY)) < 0)
	{
		perror("Cannot open jms_in pipe");
		return -1;
	}

	pid = getpid();	
	fprintf(stderr, "Console: My pid is %d.Sending it to coord\n", pid);
	sprintf(message, "%d", pid);
	write(send_fd, message, BUFSIZE);//Send coord my pid so he knows who I am

	if ( (receive_fd = open(jms_out, O_RDONLY)) < 0)
	{
		perror("Cannot open jms_out pipe");
		return -1;
	}

	read(receive_fd, message, BUFSIZE);//Receive coords pid so we can signal him for new messages (to read from pipe)
	coord_pid = atoi(message);
	fprintf(stderr, "Console: Received coord's pid (%d)\n", coord_pid);

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