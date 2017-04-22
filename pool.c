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
#include "arglist.h"

int shutdown = 0;
int pool_no;

void shutdown_handler(int x)
{
	if (PRINTS == 1)
		fprintf(stderr, "\tPool %d: Coord sent me a SIGTERM signal\n", pool_no);
	shutdown = 1;
}

int main(int argc, char* argv[])
{
	int send_fd, receive_fd;
	int pid, coord_pid;
	char message[BUFSIZE];
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

	pool_no = job_counter / max_jobs + 1;
	//Open pipes & "Handshake" with coord
	if ( (send_fd = open(send_pipe, O_WRONLY)) < 0)
	{
		perror("Cannot open jms_in pipe");
		return -1;
	}

	//Send pid to coord
	pid = getpid();
	if (PRINTS == 1)
		fprintf(stderr, "\tPool %d: My pid is %d.Sending it to coord\n", pool_no, pid);
	sprintf(message, "%d", pid);
	write(send_fd, message, BUFSIZE);

	//Open pipe for read (unblock coord)
	if ( (receive_fd = open(receive_pipe, O_RDONLY | O_NONBLOCK)) < 0)
	{
		perror("Cannot open jms_out pipe");
		return -1;
	}

	//Receive coords pid (wait for him to write it)
	while(read(receive_fd, message, BUFSIZE) == -1);
	
	coord_pid = atoi(message);
	if (PRINTS == 1)
		fprintf(stderr, "\tPool %d: Received coord's pid (%d)\n", pool_no, coord_pid);

	//set signal handler for SIGTERM (shutdown)
	signal(SIGTERM, &shutdown_handler);

	//HANDLE INCOMING MESSAGES
	int finished_jobs;
	int job_id, job_pid, job_status;
	int interrupted_jobs;
	char timestr[7];
	char datestr[9];
	job_list* j_list;
	job_node* job;
	job_info* j_info;

	j_list = job_list_create();

	finished_jobs = 0;

	//Pool finishes when max jobs jobs are finished AND we delivered all messages to coord (with break)
	//Or if shutdown was sent (SIGTERM)
	while (shutdown == 0)
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

			if (PRINTS == 1)
				fprintf(stderr, "\tPool %d: Job %d (%d) finished\n", pool_no, j_info->id, j_info->pid);
			finished_jobs++;

			//Send message to coord that this job has finished
			sprintf(message, "! %d", j_info->id);
			write(send_fd, message, BUFSIZE);

			if (finished_jobs == max_jobs)//nothing to do in this while.Move to next one
			{
				break;
			}
		}

		if (read(receive_fd, message, BUFSIZE) > 0)
		{
			if (PRINTS == 1)
				fprintf(stderr, "\tPool %d: (%s) Coord sent me: %s\n", pool_no, receive_pipe, message);
			token = strtok(message, " \t\n");//get first word of message to see the type

			if (!strcmp(token, "submit"))
			{
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
					sprintf(job_dir, "%s/sdi1400201_%d_%d_%s_%s",path, job_counter + 1, getpid(), datestr, timestr);
					mkdir(job_dir, 0777);


					//make files for output and change stdin and stderr (write to files)
					sprintf(open_dir, "%s/stdout_%d", job_dir, job_counter + 1);
					fd_stdout = open(open_dir, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

					sprintf(open_dir, "%s/stderr_%d", job_dir, job_counter + 1);
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

						perror("Invalid submit");
						exit(-5);
					}
				}

				//Make a new node for this job and add it to the joblist
				job_counter++;
				
				j_info = malloc(sizeof(job_info));
				j_info->id = job_counter;
				j_info->pid = job_pid;
				j_info->status = 0;
				job_list_add(j_list, j_info);

				free_arg_array(job_args);
				free(job_args);
				arglist_free(&arglist);

				//Reply to coord (job id and pid)
				sprintf(message, "JobID: %d, PID: %d\n", job_counter, job_pid);
			}
			else if (!strcmp(token, "suspend"))
			{
				job_id = atoi(strtok(NULL, " \t\n"));
				j_info = job_list_getby_id(j_list, job_id);

				if ( kill(j_info->pid, 19) == 0 )//if signal is sent succesfully
				{
					strcpy(message, "0");//send success message to coord
				}
				else
				{
					strcpy(message, "-1");//send fail message to coord (job is already finished)	
				}
			}
			else if (!strcmp(token, "resume"))
			{
				job_id = atoi(strtok(NULL, " \t\n"));
				j_info = job_list_getby_id(j_list, job_id);

				if ( kill(j_info->pid, 18) == 0 )//if signal is sent succesfully
				{
					strcpy(message, "0");//send success message to coord
				}
				else
				{
					strcpy(message, "-1");//send fail message to coord (job is already finished)	
				}
			}
		
			write(send_fd, message, BUFSIZE);//send message to coord
		}

	}

	/*After all jobs are finished.Coord MAY send a suspend command
	realises that the pool finished with all its jobs (specifically with the job coord wants to suspend)
	When coord receives the last message from this pool (the last job done command) he will send 'bye'
	so pool can terminate
	*/

	while (shutdown == 0)//Until coord receives the last 'JOB DONE' message.Answer with -1 (fail) to suspend commands
	{//(and no shutdown signal was received)
		if (read(receive_fd, message, BUFSIZE) > 0)
		{
			if (PRINTS == 1)
				fprintf(stderr, "\tPool %d: (%s) Coord sent me: %s\n", pool_no, receive_pipe, message);
			token = strtok(message, " \t\n");//get first word of message to see the type
			
			if (!strcmp(token, "suspend"))
			{
				strcpy(message, "-1");//send fail message to coord (job is already finished)	
				write(send_fd, message, BUFSIZE);
			}
			else if (!strcmp(token, "bye"))
			{
				break;
			}
			else//This should never happen..If it does I want to know (debug)
			{
				fprintf(stderr, "<!>Pool %d: I am done with all jobs.Nothing to do!\n", pool_no);
			}
		}
	}

	if (shutdown == 1)//if console sent shutdown (and coord sent SIGTERM)
	{
		//send a SIGTERM signal to all jobs of this poll we havent noted as finished
		job = j_list->first;
		interrupted_jobs = 0;

		while (job != NULL)
		{
			if (job->info->status != 1)//if it is not noted as finished (it is running or suspended)
			{
				if (kill(job->info->pid, 15) == 0)
				{//if SIGTERM signal was succesfully sent
					interrupted_jobs++;//count this job as interrupted
				}
				else
				{
					if (PRINTS == 1)
						fprintf(stderr, "\tPool %d: (%s) Failed to send SIGTERM to job %d.It just finished!\n",
					 pool_no, receive_pipe, job->info->id);
					j_info->status = 1;//note that this job is finished

					finished_jobs++;
					//Send message to coord that this job has finished
					sprintf(message, "! %d", j_info->id);
					write(send_fd, message, BUFSIZE);
				}

			}

			job = job->next;
		}

		//Send how many jobs were interrupted to coord
		sprintf(message, "%d", interrupted_jobs);
		write(send_fd, message, BUFSIZE);
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

	if (PRINTS == 1)
		fprintf(stderr, "\tPool %d finished:\n\t%d jobs finished,%d jobs interrupted\n",
	pool_no, finished_jobs, interrupted_jobs);

	return 0;
}