#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE 1024

#include "lists.h"
#include "functions.h"
#include "input.h"
#include "arglist.h"


int main(int argc, char* argv[])
{
	int send_fd, receive_fd;
	int pid, coord_pid;
	char message[1024];
	char job_dir[100];
	char open_dir[100];
	int fd_stdout, fd_stderr;
	char* token;
	listofargs* arglist;
	char** job_args;

	//get pipe names for communication with coord (he forked us giving us these)
	char* path = argv[1];
	char* send_pipe = argv[2];
	char* receive_pipe = argv[3];
	int job_counter = atoi(argv[4]);
	int max_jobs = atoi(argv[5]);

	int pool_no = job_counter / max_jobs + 1;
	//Open pipes & "Handshake" with coord
	if ( (send_fd = open(send_pipe, O_WRONLY)) < 0)
	{
		perror("Cannot open jms_in pipe");
		return -1;
	}

	//Send pid to coord
	pid = getpid();
	fprintf(stderr, "Pool %d: My pid is %d.Sending it to coord\n", pool_no, pid);
	sprintf(message, "%d", pid);
	write(send_fd, message, BUFSIZE);

	//Open pipe for read (unblock coord)
	if ( (receive_fd = open(receive_pipe, O_RDONLY | O_NONBLOCK)) < 0)
	{
		perror("Cannot open jms_out pipe");
		return -1;
	}

	read(receive_fd, message, BUFSIZE);//Receive coords pid
	coord_pid = atoi(message);
	fprintf(stderr, "Pool %d: Received coord's pid (%d)\n", pool_no, coord_pid);

	//HANDLE INCOMING MESSAGES
	int finished_jobs;
	int job_pid, job_status;
	char timestr[7];
	char datestr[9];
	job_list* j_list;
	job_info* j_info;

	j_list = job_list_create();

	finished_jobs = 0;
	while (finished_jobs < max_jobs)
	{
		job_pid = waitpid(-1, &job_status, WNOHANG);/*Check if any job has finished*/

		if (job_pid > 0)
		{
			j_info = job_list_getby_pid(j_list, job_pid);
			if (j_info == NULL)
			{
				fprintf(stderr, "Fatal error.Cannot find job node for job: %d\n", job_pid);
				exit(-2);
			}
			j_info->status = 1;//note that this job is finished

			fprintf(stderr, "Pool %d: Job %d (%d) finished\n", pool_no, j_info->id, j_info->pid);
			finished_jobs++;
		}

		if (read(receive_fd, message, BUFSIZE) > 0)
		{
			fprintf(stderr, "Pool %d: (%s) Coord sent me: %s\n", pool_no, receive_pipe, message);
			token = strtok(message, " \t\n");//get first word of message to see the type

			if (!strcmp(token, "submit"))
			{
				//Start making a new node in the list for this job
				job_counter++;
				
				j_info = malloc(sizeof(job_info));
				j_info->id = job_counter;
				j_info->status = 0;

				//Put arguments for the job creation (execv) in the array.Ending it with NULL!
				arglist_create(&arglist);
				token = strtok(NULL, " \t\n");

				while (token != NULL)
				{
					arglist_add(arglist, token);
					token = strtok(NULL, " \t\n");
				}

				job_args = arglist_to_array(arglist);

				if ( (job_pid = fork()) == 0)
				{
					//make directory for output
					get_date_time_str(datestr, timestr);
					sprintf(job_dir, "%s/sdi1400201_%d_%d_%s_%s",path, job_counter, getpid(), datestr, timestr);
					mkdir(job_dir, 0777);


					//make files for output and change stdin and stderr (write to files)
					sprintf(open_dir, "%s/stdout_%d", job_dir, job_counter);
					fd_stdout = open(open_dir, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

					sprintf(open_dir, "%s/stderr_%d", job_dir, job_counter);
					fd_stderr = open(open_dir, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

					dup2(fd_stdout, 1);
					dup2(fd_stderr, 2);

					//close pipes for comm with coord (jobs dont need that)
					close(send_fd);
					close(receive_fd);

					if (execvp(job_args[0], job_args) == -1)
					{
						close(fd_stdout);
						close(fd_stderr);
						
						free(j_info);
						free_arg_array(job_args);
						free(job_args);
						arglist_free(&arglist);
						job_list_free(&j_list);

						perror("Invalid submit!");
						exit(-5);
					}
				}

				free_arg_array(job_args);
				free(job_args);
				arglist_free(&arglist);

				j_info->pid = job_pid;
				j_info->start_time = time(NULL);

				job_list_add(j_list, j_info);
			}
		

		}

	}

	job_list_free(&j_list);
	close(send_fd);
	close(receive_fd);

	if ( unlink(send_pipe) < 0)
	{
		perror("Pool: Cannot unlink");
	}
	if ( unlink(receive_pipe) < 0)
	{
		perror("Pool: Cannot unlink");
	}

	fprintf(stderr, "Pool: Done!\n");

	return 0;
}