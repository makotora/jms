#include "protocol.h"

int send_message(char* message, int receiver_pid, int readfd, int writefd)
{
	//Write the message to the out pipe
	if (write(writefd, message, BUFSIZE) != BUFSIZE)
	{
		perror("send_message-write");
		return -1;
	}
	//Send receiver a signal,so he knows he just received a message and has to read the pipe
	if (kill(receiver_pid, 10) == -1)
	{
		perror("send_message-kill");
		return -2;
	}
	//Wait for receiver to (reply) write to the in pipe (his out pipe)
	if (read(readfd, message, BUFSIZE) <= 0)
	{
		fprintf(stderr, "Pipe has no writers.Cannot read!\n");
		return -3;
	}
	//Print reply
	fprintf(stderr, "%s\n", message);

	return 0;
}


int reply_message(char* reply, int writefd)
{
	write(writefd, reply, BUFSIZE);

	return 0;
}