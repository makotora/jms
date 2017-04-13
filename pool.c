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

#include "lists.h"
#include "protocol.h"
#include "input.h"

int coord_message = 0;

void receive(int signum)
{coord_message = 1;}


int main(int argc, char* argv[])
{
	char * pipe_in = NULL;
	char * pipe_out = NULL;
	int send_fd, receive_fd;
	int pid, coord_pid;
	char message[1024];

	//get pipe names for communication with coord (he forked us giving us these)
	char* send_pipe = argv[1];
	char* receive_pipe = argv[2];
	int max_jobs = atoi(argv[3]);

	//Prepare for incoming messages
	signal(SIGUSR1, &receive);//this just send global var console_message to 1 (so we know we have to read the pipe)


	//Open pipes (unblock coord)
	if ( (send_fd = open(send_pipe, O_WRONLY)) < 0)
	{
		perror("Cannot open jms_in pipe");
		return -1;
	}

	if ( (receive_fd = open(receive_pipe, O_RDONLY)) < 0)
	{
		perror("Cannot open jms_out pipe");
		return -1;
	}

	pid = getpid();
	
	//"Handshake" with coord
	fprintf(stderr, "Pool: My pid is %d.Sending it to coord\n", pid);
	sprintf(message, "%d", pid);
	write(send_fd, message, BUFSIZE);

	read(receive_fd, message, BUFSIZE);//Receive coords pid
	fprintf(stderr, "Pool: Received coord's pid (%d)\n", coord_pid);

	//HANDLE INCOMING MESSAGES
	int finished_jobs;
	int job_pid, job_status;
	job_list* j_list;
	job_info* j_info;
	j_list = job_list_create();

	finished_jobs = 0;
	while (finished_jobs < max_jobs)
	{
		job_pid = waitpid(-1, &job_status, WNOHANG);/*Check if any job has finished*/

		if (job_pid > 0)
		{
			j_info = job_list_getpid(j_list, job_pid);
			if (j_info == NULL)
			{
				fprintf(stderr, "Fatal error.Cannot find job node for job: %d\n", job_pid);
				exit(-2);
			}
			j_info->status = 1;//note that this job is finished

			fprintf(stderr, "Job %d (%d) finished\n", j_info->id, j_info->pid);
		}

		if (coord_message == 1)
		{
			coord_message = 0;
		}

		sleep(5);
		break;
	}

	close(send_fd);
	close(receive_fd);

	if ( unlink(send_pipe) < 0)
	{
		perror("Console: Cannot unlink");
	}
	if ( unlink(receive_pipe) < 0)
	{
		perror("Console: Cannot unlink");
	}

	fprintf(stderr, "Pool: Done!\n");

	return 0;
}